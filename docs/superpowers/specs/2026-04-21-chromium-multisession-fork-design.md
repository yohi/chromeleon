# Chromium マルチセッション・アンチ指紋ブラウザ — 統合アーキテクチャ Spec

- **作成日:** 2026-04-21
- **スコープ種別:** 統合アーキテクチャ Spec（後続の実装Specは本Specを前提に分割）
- **対象Chromium:** stable M135系（`config/chromium_version` でtag pin）
- **対象プラットフォーム:** Linux のみ
- **Base フォーク戦略:** パッチセット方式（Brave式 overlay + 薄いフック）

## 0. 本Specの位置づけ

本ドキュメントは、Chromiumを土台としたカスタムブラウザの**統合アーキテクチャ**を定義する。以下の5サブシステムを単一のフォーク構造上に収めるための境界・規約・ビルド基盤を規定する。

1. **A. EphemeralSessionManager** — 動的 `StoragePartition` / プロファイル分離
2. **B. FingerprintNoiseSource** — Blink / V8 レベルの webdriver / Canvas / WebGL 加工
3. **C. TabGridView** — Chromium Views による N×M グリッド表示
4. **D. MultiSessionOpenDialog** — `RenderViewContextMenu` 拡張 + モーダル入力
5. **E. PartitionExtensionAutoloader** — 全 `StoragePartition` への MV3 拡張の共通自動有効化

個別の実装Spec（Spec-A〜Spec-F）は、本Specが示す境界を守った上で独立に進める。

## 1. アーキテクチャ概要

### 1.1 コンポーネント図

```
┌───────────────────────────────────────────────────────────────────┐
│  Browser Process                                                   │
│                                                                    │
│  UI Layer (UI Thread / Views)                                      │
│    TabGridView ─── MultiSessionOpenDialog ─── GridToggleButton     │
│         ▲                   ▲                                       │
│         │ observes          │ OK → ExpandLinkInSessions            │
│    RenderViewContextMenu (patched: AppendLinkItems 末尾)            │
│                             │                                      │
│  Session Core (UI Thread, KeyedService on Profile)                 │
│    EphemeralSessionManager ───────────────────────────┐            │
│      - CreateSessionForNewTab()                        │            │
│      - DestroySessionForTab(wc)                        │            │
│      - GetSeedForPartitionConfig(config)               │            │
│      - ExpandLinkInSessions(url, n)                    │            │
│         │ owns (1:N)                                   │            │
│         ▼                                              │ observer   │
│    StoragePartitionConfig{in_memory=true, name=uuid}   │            │
│         │ passed to BrowserContext::GetStoragePartition│            │
│                                                        ▼            │
│  Storage/Network (IO Thread / Network Service)    Extension Autoloader
│    StoragePartitionImpl (ephemeral per session)   (UI Thread)       │
│    CookieManager / CacheStorage / IndexedDB                         │
│    すべて in-memory                                                 │
└───────────────────────────────────────────────────────────────────┘

            ▲ Mojo (blink.mojom.FingerprintSeedReceiver)
            │
┌───────────┴───────────────────────────────────────────────────────┐
│  Renderer Process (per session, per site)                         │
│    Blink / V8 (Renderer Main Thread)                              │
│      FingerprintNoiseSource (Supplement<LocalDOMWindow>)          │
│        - SetSeed(uint64)  ← Mojo                                  │
│        - ApplyCanvasNoise(ImageData*)                             │
│        - ApplyWebGLNoise(pixels, format, type)                    │
│        - WebDriverEnabled() → const false                         │
│      ↑ called from:                                               │
│        - Canvas2D readback (toDataURL / toBlob / getImageData)    │
│        - WebGL readPixels                                         │
│        - Navigator::webdriver()                                   │
└───────────────────────────────────────────────────────────────────┘
```

### 1.2 コンポーネント責務表

| モジュール | 責務（1行） | 所有者 | 所属スレッド | 所在 |
|---|---|---|---|---|
| A. EphemeralSessionManager | タブへ独立 `StoragePartition` を割当・破棄 | `Profile` (KeyedService) | UI | `overlay/chrome/browser/multi_session/` |
| B. FingerprintNoiseSource | Blink で webdriver / Canvas / WebGL 値を加工 | `LocalDOMWindow` (Supplement) | Renderer Main | `overlay/third_party/blink/renderer/modules/multi_session_fp/` |
| C. TabGridView | 全タブを N×M タイル表示 + ページング | `BrowserView` | UI | `overlay/chrome/browser/ui/views/tab_grid/` |
| D. MultiSessionOpenDialog + MenuItem | リンクを N 独立セッションで一括展開 | `BrowserView` / `RenderViewContextMenu` | UI | `overlay/chrome/browser/ui/views/multi_session_dialog/` + patches |
| E. PartitionExtensionAutoloader | 新規パーティションに指定拡張を自動ロード | `Profile` (KeyedService) | UI | `overlay/chrome/browser/extensions/partition_extension_autoloader/` |

### 1.3 主要データフロー

**フロー1: 「このリンクを5セッションで開く」**

```
ユーザがリンク右クリック
  → RenderViewContextMenu::AppendLinkItems (patched) が MULTI_SESSION 項目追加
  → ユーザ選択 → MultiSessionOpenDialog::Show(link_url)
  → "5" 入力 → OK
  → EphemeralSessionManager::ExpandLinkInSessions(url, 5)
      ├─ 5回ループ:
      │   ├─ CreateSessionForNewTab() → SessionHandle{uuid, seed, config}
      │   ├─ PartitionExtensionAutoloader::OnPartitionCreated() → 拡張自動ロード
      │   ├─ NavigateParams{storage_partition_config=config, NEW_BACKGROUND_TAB}
      │   └─ Navigate(&params)
      └─ tab strip に 5 タブ追加
```

**フロー2: Canvas readback のフィンガープリント加工**

```
JS: canvas.toDataURL()
  → Blink Renderer Main Thread: CanvasRenderingContext2D::toDataURL
  → [hook] FingerprintNoiseSource::ApplyCanvasNoise(image_data)
      ├─ seed: Mojo で frame init 時に受信・キャッシュ済み
      ├─ Mulberry32(seed XOR pixel_index) → 決定的 PRNG
      └─ R/G/B 各chに ±1 LSB の決定的摂動
  → Encoded → JS に返却
```

## 2. スレッドモデルとライフサイクル

### 2.1 スレッド所属マトリクス

| 処理 | 実行スレッド | 生成タイミング | 破棄タイミング |
|---|---|---|---|
| EphemeralSessionManager | UI | Profile起動 (`BrowserContextKeyedServiceFactory`) | Profile shutdown |
| StoragePartitionConfig 決定 | UI | `NavigateParams`構築時 | 値型、GCなし |
| StoragePartitionImpl 生成 | UI 起動 → Network/Storage Service (別プロセス) は Mojo | 初回 `GetStoragePartition()` | `BrowserContext`破棄 |
| PartitionExtensionAutoloader | UI のみ | Profile起動 | Profile shutdown |
| TabGridView / MultiSessionOpenDialog / GridToggleButton | UI のみ (Views) | `BrowserView`構築 / ユーザ操作 | ウィンドウclose / Dialog終了 |
| ThumbnailCapturer | UI が起動 → GPU process でレンダ → UI callback | Grid表示時 | Grid退出時 |
| FingerprintNoiseSource | Renderer Main のみ | `ExecutionContext`生成 | `ExecutionContext`破棄 |
| Seed 配信 | UI (送信) → Renderer Main (受信) | `RenderFrameCreated` / `DidCreateNewDocument` | 1回、以降キャッシュ |
| Canvas/WebGL ノイズ適用 | Renderer Main | readback呼出毎 | 毎回 |
| `navigator.webdriver` 返却 | Renderer Main | getter呼出毎 | 毎回（定数 false） |

### 2.2 スレッド規約（肯定形）

- **UI側処理は全て `content::BrowserThread::UI` で実行する** — `DCHECK_CURRENTLY_ON(BrowserThread::UI)` で自己検証
- **Renderer側処理は全て `blink::Thread::MainThread()` で実行する** — `DCHECK(IsMainThread())` で自己検証
- **スレッド跨ぎの値受け渡しは Mojo または `base::PostTask` のみを使う**
- **seed は `uint64_t` の値型のみを跨がせる** — ポインタやCallbackは跨がせない
- **IO thread へは直接触れず、`content/` に委譲する**

### 2.3 主要ライフサイクル

- **セッション分離**: タブクローズ → `WebContentsDestroyed` → `DestroySessionForTab` → `StoragePartition` 解放（in-memoryストアはメモリから即消滅）
- **アンチ指紋**: Renderer起動は `StoragePartition` ごとに独立（Chromium既定）。`SetSeed` は `RenderFrameCreated` 毎に呼ぶ（同一Partitionは同seed、別Partitionは別seed）
- **Grid UI**: `BrowserView`と同寿命。`SetVisible(false)` でOFF維持。サムネイル取得はcallback到着までキャッシュ表示でUIをブロックしない
- **多重セッション展開**: N回の `CreateSessionForNewTab` はUI threadで連続実行。非同期初期化は `content/` に委譲
- **拡張自動ロード**: `PartitionExtensionAutoloader` は `EphemeralSessionManager::Observer` として登録、`OnPartitionCreated` で Pref の `kAutoEnabledExtensionIds` を読んで逐次有効化

### 2.4 初期スコープ外として明示する項目（肯定的宣言）

- **OffscreenCanvas (Worker上のCanvas)**: 初期スコープではMainスレッド上のCanvas/WebGLのみを対象とする。Workerへの拡張は後続Specで扱う
- **UA/Screen/Audio/Font/WebRTC 等のフィンガープリント対策**: 初期スコープでは webdriver / Canvas / WebGL の3点に限定する

## 3. リポジトリ構造とビルドシステム

### 3.1 ディレクトリ構造

```
/ (project root, git repo "custom-chromium")
├── .devcontainer/
│   ├── devcontainer.json
│   ├── Dockerfile
│   └── post-create.sh
├── .github/workflows/
│   ├── pr-lint.yml
│   └── nightly-build.yml
├── patches/                     # 薄いフックパッチ (数十行/枚)
│   ├── 0001-content-storage-partition-hook.patch
│   ├── 0002-blink-navigator-webdriver-hook.patch
│   ├── 0003-blink-canvas-readback-hook.patch
│   ├── 0004-blink-webgl-readpixels-hook.patch
│   ├── 0005-chrome-render-view-context-menu-hook.patch
│   ├── 0006-chrome-browser-view-grid-toggle-hook.patch
│   ├── 0007-extensions-partition-load-hook.patch
│   └── series
├── chromium_src/                # 追加ファイル (パッチではない)
│   └── overlay/
│       ├── chrome/browser/
│       │   ├── multi_session/
│       │   ├── extensions/partition_extension_autoloader/
│       │   └── ui/views/tab_grid/
│       │   └── ui/views/multi_session_dialog/
│       ├── third_party/blink/renderer/modules/multi_session_fp/
│       └── public/mojom/multi_session/fingerprint.mojom
├── build/
│   ├── apply_patches.py
│   ├── unapply_patches.py
│   ├── sync_overlay.py
│   ├── overlay_gn.patch         # 唯一のGN侵襲パッチ
│   ├── build_wrapper.sh
│   └── run_unit_tests.sh
├── config/
│   ├── chromium_version
│   └── gn_args.gn
├── docs/superpowers/specs/
└── README.md
```

### 3.2 Devcontainer (`.devcontainer/devcontainer.json`)

```jsonc
{
  "name": "custom-chromium-dev",
  "build": { "dockerfile": "Dockerfile" },
  "runArgs": [
    "--cap-add=SYS_PTRACE",
    "--security-opt", "seccomp=unconfined",
    "--shm-size=2g"
  ],
  "mounts": [
    "source=chromium-src,target=/workspaces/chromium/src,type=volume",
    "source=chromium-out,target=/workspaces/chromium/out,type=volume",
    "source=chromium-depot-tools,target=/workspaces/chromium/depot_tools,type=volume"
  ],
  "workspaceMount": "source=${localWorkspaceFolder},target=/workspaces/custom-chromium,type=bind",
  "workspaceFolder": "/workspaces/custom-chromium",
  "containerEnv": {
    "CHROMIUM_SRC": "/workspaces/chromium/src",
    "CHROMIUM_OUT": "/workspaces/chromium/out/Default",
    "DEPOT_TOOLS_PATH": "/workspaces/chromium/depot_tools",
    "PATH": "/workspaces/chromium/depot_tools:${PATH}",
    "CCACHE_DIR": "/workspaces/chromium/out/.ccache"
  },
  "postCreateCommand": ".devcontainer/post-create.sh",
  "customizations": {
    "vscode": {
      "extensions": [
        "llvm-vs-code-extensions.vscode-clangd",
        "ms-vscode.cpptools"
      ],
      "settings": {
        "clangd.arguments": ["--compile-commands-dir=/workspaces/chromium/out/Default"]
      }
    }
  }
}
```

### 3.3 Dockerfile

```dockerfile
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Tokyo
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential pkg-config lsb-release sudo \
      python3 python3-pip python3-venv \
      git curl ca-certificates gnupg \
      lld clang clang-tidy clang-format clangd \
      ninja-build cmake ccache \
      file tzdata locales \
 && rm -rf /var/lib/apt/lists/*
ARG USER_UID=1000
ARG USER_GID=1000
RUN groupadd --gid ${USER_GID} dev \
 && useradd --uid ${USER_UID} --gid ${USER_GID} -m -s /bin/bash dev \
 && echo 'dev ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/dev
USER dev
WORKDIR /workspaces/custom-chromium
```

### 3.4 post-create.sh（抜粋）

```bash
#!/usr/bin/env bash
set -euo pipefail

if [ ! -d "${DEPOT_TOOLS_PATH}/.git" ]; then
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
      "${DEPOT_TOOLS_PATH}"
fi

if [ ! -d "${CHROMIUM_SRC}/.git" ]; then
  mkdir -p "$(dirname "${CHROMIUM_SRC}")"
  cd "$(dirname "${CHROMIUM_SRC}")"
  fetch --nohooks --no-history chromium
  cd src
  CHROMIUM_VERSION=$(cat /workspaces/custom-chromium/config/chromium_version)
  git checkout "tags/${CHROMIUM_VERSION}" -b "work/${CHROMIUM_VERSION}"
  gclient sync --with_branch_heads --with_tags -D
  build/install-build-deps.sh --no-prompt
  gclient runhooks
fi

python3 /workspaces/custom-chromium/build/sync_overlay.py
python3 /workspaces/custom-chromium/build/apply_patches.py
```

### 3.5 GN侵襲の最小化: `build/overlay_gn.patch`

**`//chrome/browser/BUILD.gn` に追記する唯一のブロック:**

```gn
group("multi_session_overlay_all") {
  deps = [
    "//chromium_src/overlay/chrome/browser/multi_session",
    "//chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader",
    "//chromium_src/overlay/chrome/browser/ui/views/tab_grid",
    "//chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog",
  ]
}

source_set("browser") {
  # ... 既存 ...
  deps += [ ":multi_session_overlay_all" ]
}
```

**`//third_party/blink/renderer/modules/BUILD.gn`:**

```gn
deps += [ "//chromium_src/overlay/third_party/blink/renderer/modules/multi_session_fp" ]
```

### 3.6 overlay 注入メカニズム: `build/sync_overlay.py`

GNは `//src/...` 外のパスを参照できないため、`chromium_src/overlay/` を `$CHROMIUM_SRC/chromium_src/overlay/` に **symlink** で載せる（コピーではない）。編集は自リポジトリ側1箇所で完結する。

```python
OVERLAY_ROOT = Path("/workspaces/custom-chromium/chromium_src/overlay")
CHROMIUM_SRC = Path(os.environ["CHROMIUM_SRC"])
TARGET = CHROMIUM_SRC / "chromium_src" / "overlay"

if TARGET.is_symlink(): TARGET.unlink()
elif TARGET.exists(): shutil.rmtree(TARGET)
TARGET.parent.mkdir(exist_ok=True, parents=True)
TARGET.symlink_to(OVERLAY_ROOT)
```

### 3.7 パッチ適用: `build/apply_patches.py`

```python
SERIES = Path("patches/series").read_text().splitlines()
for patch_name in SERIES:
    subprocess.run(
        ["git", "apply", "--3way", str(Path("patches") / patch_name)],
        cwd=os.environ["CHROMIUM_SRC"],
        check=True,
    )
```

### 3.8 リベース戦略（月次）

1. `config/chromium_version` を新Milestoneに更新
2. `gclient sync`
3. `apply_patches.py --interactive` で失敗パッチを確認
4. 失敗パッチを `git am --3way` / `quilt refresh` で手修正
5. `sync_overlay.py` は常に symlink で不変
6. `autoninja` ビルド確認 → unit_tests 緑確認 → `chore(rebase): M<ver>` でコミット

**パッチ最小化の指針:**
- パッチは挿入位置の目印となる関数コメント直下にフックを置く
- パッチ1枚あたり **50行以内** を目安、超える場合はoverlayに切り出す

## 4. 主要 C++ インタフェース

> **本章のdiff記法について:** `@@ -XXX,6 +XXX,10 @@` の `XXX` は上流バージョンごとに変動する行番号のプレースホルダである。実装Spec段階で固定対象のChromiumタグ（`config/chromium_version`）における実行番号を埋める。

### 4.1 モジュール A: EphemeralSessionManager

**`overlay/chrome/browser/multi_session/ephemeral_session_manager.h`**

```cpp
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_EPHEMERAL_SESSION_MANAGER_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_EPHEMERAL_SESSION_MANAGER_H_

#include <string>
#include <unordered_map>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/storage_partition_config.h"

class Profile;
namespace content { class StoragePartition; class WebContents; }

namespace multi_session {

// SessionHandle は StoragePartition 本体ポインタを保持しない。
// 本体が必要な呼出側は profile_->GetStoragePartition(handle.config) で
// 都度解決する。これにより BrowserContext 破棄順序や明示的 partition
// 解放経路での dangling pointer リスクを排除する。
struct SessionHandle {
  std::string partition_id;  // config.partition_name と等しい
  uint64_t fingerprint_seed;
  content::StoragePartitionConfig config;
};

class EphemeralSessionManager : public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPartitionCreated(const SessionHandle& handle) {}
    virtual void OnPartitionDestroyed(const std::string& partition_id) {}
  };

  explicit EphemeralSessionManager(Profile* profile);
  ~EphemeralSessionManager() override;
  EphemeralSessionManager(const EphemeralSessionManager&) = delete;
  EphemeralSessionManager& operator=(const EphemeralSessionManager&) = delete;

  SessionHandle CreateSessionForNewTab();
  content::StoragePartitionConfig PartitionConfigFor(
      const SessionHandle& handle) const;
  void DestroySessionForTab(content::WebContents* wc);

  // 呼出側は任意の StoragePartition* から GetConfig() を取得して渡す。
  // StoragePartition 本体は本クラスで保持しない（lifetime安全性のため）。
  uint64_t GetSeedForPartitionConfig(
      const content::StoragePartitionConfig& config) const;

  void ExpandLinkInSessions(const GURL& link_url, int num_sessions);

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);

 private:
  // Entry は seed のみを保持する。partition 本体は profile_ 経由で都度解決する。
  struct Entry {
    uint64_t seed;
    content::StoragePartitionConfig config;
  };
  // Key: partition_id (= config.partition_name)
  std::unordered_map<std::string, Entry> sessions_;
  raw_ptr<Profile> profile_;
  base::ObserverList<Observer> observers_;
};

}  // namespace multi_session
#endif
```

**Factory:**

```cpp
class EphemeralSessionManagerFactory : public ProfileKeyedServiceFactory {
 public:
  static EphemeralSessionManager* GetForProfile(Profile* profile);
  static EphemeralSessionManagerFactory* GetInstance();
 private:
  EphemeralSessionManagerFactory();
  KeyedService* BuildServiceInstanceFor(BrowserContext* ctx) const override;
};
```

**`CreateSessionForNewTab()` の核ロジック:**

`GetStoragePartition()` は副作用としてパーティション実体を生成するために1回だけ呼び、戻り値のポインタは保存しない。以降 partition 本体が必要な箇所は `profile_->GetStoragePartition(handle.config)` で都度解決する（cached lookup なのでコストは小）。

```cpp
SessionHandle EphemeralSessionManager::CreateSessionForNewTab() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const std::string pid = base::Uuid::GenerateRandomV4().AsLowercaseString();
  const uint64_t seed = base::RandUint64();

  auto config = content::StoragePartitionConfig::Create(
      profile_, /*partition_domain=*/"multi_session",
      /*partition_name=*/pid, /*in_memory=*/true);

  // 実体を生成するためだけに呼ぶ。返り値はあえて保持しない。
  profile_->GetStoragePartition(config, /*can_create=*/true);

  sessions_.emplace(pid, Entry{seed, config});
  SessionHandle h{pid, seed, config};
  for (auto& obs : observers_) obs.OnPartitionCreated(h);
  return h;
}
```

**`GetSeedForPartitionConfig()` の核ロジック:**

```cpp
uint64_t EphemeralSessionManager::GetSeedForPartitionConfig(
    const content::StoragePartitionConfig& config) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const auto it = sessions_.find(config.partition_name());
  if (it == sessions_.end()) return 0;
  // partition_name が偶然衝突した場合に備え、config全体で一致確認を行う。
  if (it->second.config != config) return 0;
  return it->second.seed;
}
```

**`ExpandLinkInSessions()`:**

```cpp
void EphemeralSessionManager::ExpandLinkInSessions(const GURL& url, int n) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (int i = 0; i < n; ++i) {
    SessionHandle h = CreateSessionForNewTab();
    NavigateParams params(profile_, url, ui::PAGE_TRANSITION_LINK);
    params.disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
    params.storage_partition_config = PartitionConfigFor(h);
    Navigate(&params);
  }
}
```

### 4.2 モジュール B: FingerprintNoiseSource

**Mojo IDL (`overlay/public/mojom/multi_session/fingerprint.mojom`):**

```
module blink.mojom;

interface FingerprintSeedReceiver {
  SetSeed(uint64 seed);
};
```

**Browser側配信 (`overlay/chrome/browser/multi_session/fingerprint_seed_delivery.{h,cc}`):**

```cpp
class FingerprintSeedDelivery : public content::WebContentsObserver {
 public:
  explicit FingerprintSeedDelivery(content::WebContents* wc);
  void RenderFrameCreated(content::RenderFrameHost* rfh) override;
};

void FingerprintSeedDelivery::RenderFrameCreated(
    content::RenderFrameHost* rfh) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* mgr = EphemeralSessionManagerFactory::GetForProfile(
      Profile::FromBrowserContext(rfh->GetBrowserContext()));
  if (!mgr) return;

  // StoragePartition 本体ポインタは保持せず、config 値のみを Manager に渡す。
  const content::StoragePartitionConfig& config =
      rfh->GetStoragePartition()->GetConfig();
  const uint64_t seed = mgr->GetSeedForPartitionConfig(config);
  if (seed == 0) return;  // 非エフェメラル（既定 Profile）パーティションは対象外

  mojo::AssociatedRemote<blink::mojom::FingerprintSeedReceiver> remote;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&remote);
  remote->SetSeed(seed);
}
```

**Renderer側 (`overlay/third_party/blink/renderer/modules/multi_session_fp/fingerprint_noise_source.h`):**

```cpp
namespace blink {

class CORE_EXPORT FingerprintNoiseSource final
    : public GarbageCollected<FingerprintNoiseSource>,
      public Supplement<LocalDOMWindow>,
      public mojom::FingerprintSeedReceiver {
 public:
  static const char kSupplementName[];
  static FingerprintNoiseSource& From(LocalDOMWindow& window);

  explicit FingerprintNoiseSource(LocalDOMWindow& window);

  void SetSeed(uint64_t seed) override;
  void ApplyCanvasNoise(ImageData* data) const;
  void ApplyWebGLNoise(base::span<uint8_t> pixels,
                       GLenum format,
                       GLenum type) const;
  static bool WebDriverEnabled() { return false; }

  void Trace(Visitor*) const override;

 private:
  uint64_t seed_ = 0;
  bool seed_received_ = false;
  HeapMojoAssociatedReceiver<mojom::FingerprintSeedReceiver, FingerprintNoiseSource>
      receiver_;
};

}  // namespace blink
```

**PRNG (allocation-free, deterministic):**

```cpp
class Mulberry32 {
 public:
  explicit Mulberry32(uint32_t seed) : state_(seed) {}
  uint32_t Next() {
    uint32_t z = (state_ += 0x6D2B79F5);
    z = (z ^ (z >> 15)) * (z | 1);
    z ^= z + (z ^ (z >> 7)) * (z | 61);
    return z ^ (z >> 14);
  }
 private:
  uint32_t state_;
};
```

**Canvas ノイズ適用:**

```cpp
void FingerprintNoiseSource::ApplyCanvasNoise(ImageData* data) const {
  if (!seed_received_) return;
  const uint32_t base = static_cast<uint32_t>(seed_);
  auto bytes = data->data()->Data();
  for (size_t i = 0; i < data->data()->length(); i += 4) {
    Mulberry32 rng(base ^ static_cast<uint32_t>(i));
    for (int c = 0; c < 3; ++c) {  // R,G,B のみ (Alphaは触らない)
      const int delta = static_cast<int>(rng.Next() & 1) * 2 - 1;
      const int v = static_cast<int>(bytes[i + c]) + delta;
      bytes[i + c] = static_cast<uint8_t>(std::clamp(v, 0, 255));
    }
  }
}
```

### 4.3 モジュール C: TabGridView

```cpp
class TabGridView : public views::View,
                    public TabStripModelObserver {
 public:
  explicit TabGridView(BrowserView* browser_view);
  ~TabGridView() override;

  void ToggleVisibility();

  // views::View
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(const views::SizeBounds&) const override;

  // TabStripModelObserver
  void OnTabStripModelChanged(TabStripModel* model,
                              const TabStripModelChange& change,
                              const TabStripSelectionChange& sel) override;

 private:
  void BuildPage(int page_index);
  void OnTileClicked(int tab_index);
  void OnPageButtonClicked(int delta);

  raw_ptr<BrowserView> browser_view_;
  raw_ptr<TabGridPageIndicator> page_indicator_;
  std::vector<raw_ptr<TabGridTile>> tiles_;
  int rows_ = 4;
  int cols_ = 3;
  int current_page_ = 0;
};
```

**起動ボタン注入 (`patches/0006-...`):**

```diff
--- a/chrome/browser/ui/views/toolbar/toolbar_view.cc
+++ b/chrome/browser/ui/views/toolbar/toolbar_view.cc
@@ -XXX,6 +XXX,10 @@ void ToolbarView::Init() {
   AddChildView(std::move(location_bar_));
+  // MULTI_SESSION_HOOK: add Grid toggle button right after URL bar.
+  AddChildView(std::make_unique<multi_session::GridToggleButton>(
+      browser_view_,
+      base::BindRepeating(&BrowserView::ToggleTabGrid, browser_view_)));
   AddChildView(std::move(app_menu_button_));
```

### 4.4 モジュール D: MultiSessionOpenDialog

```cpp
class MultiSessionOpenDialog : public views::DialogDelegateView {
 public:
  static void Show(Browser* browser, const GURL& link_url);

  MultiSessionOpenDialog(Browser* browser, const GURL& link_url);
  ~MultiSessionOpenDialog() override;

  std::u16string GetWindowTitle() const override;
  bool Accept() override;

 private:
  raw_ptr<Browser> browser_;
  GURL link_url_;
  raw_ptr<views::Textfield> count_field_;
};

bool MultiSessionOpenDialog::Accept() {
  int n = 0;
  if (!base::StringToInt(count_field_->GetText(), &n)) return false;
  n = std::clamp(n, 1, 20);
  auto* mgr = EphemeralSessionManagerFactory::GetForProfile(
      browser_->profile());
  mgr->ExpandLinkInSessions(link_url_, n);
  return true;
}
```

**コンテキストメニュー注入 (`patches/0005-...`):**

```diff
--- a/chrome/browser/renderer_context_menu/render_view_context_menu.cc
+++ b/chrome/browser/renderer_context_menu/render_view_context_menu.cc
@@ -XXX,6 +XXX,9 @@ void RenderViewContextMenu::AppendLinkItems() {
   menu_model_.AddItemWithStringId(
       IDC_CONTENT_CONTEXT_OPENLINKNEWTAB, IDS_CONTENT_CONTEXT_OPENLINKNEWTAB);
+  // MULTI_SESSION_HOOK
+  menu_model_.AddItemWithStringId(
+      IDC_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION,
+      IDS_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION);
 }
@@ -XXX,6 +XXX,13 @@ void RenderViewContextMenu::ExecuteCommand(int id, int flags) {
   switch (id) {
+    case IDC_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION: {
+      multi_session::MultiSessionOpenDialog::Show(
+          chrome::FindBrowserWithTab(source_web_contents_),
+          params_.link_url);
+      return;
+    }
     case IDC_CONTENT_CONTEXT_OPENLINKNEWTAB: { /* ... */ }
```

### 4.5 モジュール E: PartitionExtensionAutoloader

```cpp
class PartitionExtensionAutoloader
    : public KeyedService,
      public multi_session::EphemeralSessionManager::Observer {
 public:
  explicit PartitionExtensionAutoloader(Profile* profile);
  ~PartitionExtensionAutoloader() override;

  void OnPartitionCreated(
      const multi_session::SessionHandle& handle) override;

 private:
  std::vector<std::string> GetAutoEnabledExtensionIds() const;
  raw_ptr<Profile> profile_;
};

void PartitionExtensionAutoloader::OnPartitionCreated(
    const multi_session::SessionHandle& h) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const auto ids = GetAutoEnabledExtensionIds();
  auto* service = extensions::ExtensionSystem::Get(profile_)->extension_service();
  auto* registry = extensions::ExtensionRegistry::Get(profile_);
  for (const auto& id : ids) {
    const auto* ext = registry->GetInstalledExtension(id);
    if (!ext) continue;
    service->EnableExtensionForPartition(id, h.partition_id);
  }
}
```

**PrefService 整合:** 新pref `kAutoEnabledExtensionIds` (`base::Value::List<std::string>`) を `chrome/browser/profiles/profile_impl.cc` のpref登録に追加する。

### 4.6 フックパッチ一覧（確定版）

| # | ファイル | 行数目安 | 目的 |
|---|---|---|---|
| 0001 | `content/browser/storage_partition_impl_map.cc` | ~5 | 生成時に `EphemeralSessionManager` へ通知 |
| 0002 | `third_party/blink/renderer/core/frame/navigator.cc` | ~3 | `Navigator::webdriver()` 定数化 |
| 0003 | `third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.cc` | ~8 | readback 後にノイズ適用 |
| 0004 | `third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc` | ~8 | readPixels 後にノイズ適用 |
| 0005 | `chrome/browser/renderer_context_menu/render_view_context_menu.cc` | ~15 | メニュー項目 + コマンド分岐 |
| 0006 | `chrome/browser/ui/views/toolbar/toolbar_view.cc` | ~5 | Grid toggle button 注入 |
| 0007 | `chrome/browser/extensions/extension_service.cc` | ~10 | `EnableExtensionForPartition` スタブ |
| `overlay_gn.patch` | `chrome/browser/BUILD.gn`, `third_party/blink/renderer/modules/BUILD.gn` | ~10 | overlay ターゲット接続 |

**合計: ~65行の上流改変**。全て既存関数末尾への挿入、または新コマンドID分岐で、上流ロジックの書き換えはゼロ。

## 5. ビルド・テスト手順

### 5.1 初回セットアップ

```bash
# ホスト
# <custom-chromium-repo> は本プロジェクトのGitリモートURLで置換する
git clone <custom-chromium-repo> && cd custom-chromium
devcontainer up --workspace-folder .

# Devcontainer 内 (post-create.sh が自動実行される)
#   - depot_tools 取得
#   - chromium/src を tagged version で fetch
#   - build/install-build-deps.sh 実行
#   - sync_overlay.py でsymlink
#   - apply_patches.py でパッチ適用
# 所要: 40分〜2時間 (回線依存)
```

### 5.2 ビルド

```bash
cd $CHROMIUM_SRC
gn gen out/Default --args="$(cat /workspaces/custom-chromium/config/gn_args.gn)"
autoninja -C out/Default chrome
out/Default/chrome --enable-features=MultiSessionTabs --user-data-dir=/tmp/chromium-profile
```

**`config/gn_args.gn`:**

```gn
is_debug = false
is_component_build = true
symbol_level = 1
enable_nacl = false
use_goma = false
use_remoteexec = false
cc_wrapper = "ccache"
blink_symbol_level = 1
is_official_build = false
proprietary_codecs = false
ffmpeg_branding = "Chromium"
```

### 5.3 テスト実行

```bash
# 全 overlay モジュールの unit_tests
autoninja -C out/Default multi_session_overlay_tests
bash /workspaces/custom-chromium/build/run_unit_tests.sh

# 個別
autoninja -C out/Default ephemeral_session_manager_unittest
out/Default/ephemeral_session_manager_unittest
```

**モジュール別 unit_test カバー範囲:**

| モジュール | テスト対象 | 使用フェイク |
|---|---|---|
| EphemeralSessionManager | セッション生成、ID一意性、破棄、`GetSeedForPartitionConfig`、`ExpandLinkInSessions` | `TestingProfile`, `MockBrowserContext` |
| FingerprintNoiseSource | PRNG決定性、同seed同入力→同出力、異seed→差分、`webdriver()`定数 | `V8TestingScope`, `ImageData` fixtures |
| TabGridView | rows×cols レイアウト、ページング境界、タイルクリック時の `ActivateTabAt` 呼出 | `TestBrowserView`, `TestTabStripModel` |
| MultiSessionOpenDialog | 1〜20 clamp、OK時の `ExpandLinkInSessions` 呼出 | `MockEphemeralSessionManager` |
| PartitionExtensionAutoloader | pref読出、新規パーティション通知→拡張ロード呼出 | `TestExtensionRegistry`, `FakePrefService` |

### 5.4 静的解析

```bash
git -C $CHROMIUM_SRC cl format --upstream-to=HEAD~1
tools/clang/scripts/run_tool.py --tool clang-tidy --all chromium_src/overlay/
gn check out/Default //chromium_src/overlay/...
```

### 5.5 GitHub Actions — `pr-lint.yml`

```yaml
name: pr-lint
on: pull_request
jobs:
  lint:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with: { python-version: "3.12" }
      - name: Install clang-format + clang-tidy
        run: sudo apt-get install -y clang-format-15 clang-tidy-15
      - name: clang-format check (overlay)
        run: find chromium_src/overlay -name '*.cc' -o -name '*.h' |
             xargs clang-format-15 --dry-run --Werror
      - name: Patch dry-run
        run: python3 build/ci_patch_dryrun.py
      - name: Python lint
        run: pip install ruff && ruff check build/
      - name: Spec markdown lint
        run: npx markdownlint-cli2 'docs/**/*.md'
```

### 5.6 GitHub Actions — `nightly-build.yml`

```yaml
name: nightly-build
on:
  schedule: [ { cron: "0 17 * * *" } ]  # JST 02:00
  workflow_dispatch: {}
jobs:
  build-and-test:
    runs-on: [self-hosted, linux, chromium-builder]
    timeout-minutes: 360
    steps:
      - uses: actions/checkout@v4
      - name: gclient sync
        run: cd $CHROMIUM_SRC && gclient sync -D --force
      - name: Sync overlay + apply patches
        run: |
          python3 build/sync_overlay.py
          python3 build/apply_patches.py
      - name: GN gen
        run: cd $CHROMIUM_SRC && gn gen out/Default --args="$(cat ../../config/gn_args.gn)"
      - name: Build overlay unit tests
        run: cd $CHROMIUM_SRC && autoninja -C out/Default multi_session_overlay_tests
      - name: Run unit tests
        run: bash build/run_unit_tests.sh
      - uses: actions/upload-artifact@v4
        if: always()
        with: { name: logs, path: $CHROMIUM_SRC/out/Default/*.log }
```

## 6. 受け入れ基準（肯定形）

以下がすべて真なら「完了」とみなす。

| 要件 | 検証基準 | 検証手段 |
|---|---|---|
| R1 セッション分離 | 2つの独立タブで同ドメインにログインすると、各タブのCookie/IndexedDB/cacheが異なるストアに保存される | `ephemeral_session_manager_unittest.cc` + 手動E2E |
| R1 セッション分離 | タブを閉じた直後、対応する `StoragePartition` が `GetStoragePartitions()` 返却リストから消える | unit_test: `DestroySessionRemovesPartition` |
| R2 webdriver | Chromium起動後、任意タブで `navigator.webdriver` が `false` を返す | `navigator_webdriver_unittest.cc` + 手動JS |
| R2 Canvas | 同一canvasを同一セッションで2回読取 → ハッシュ一致 | `fingerprint_noise_source_test.cc` |
| R2 Canvas | 同一canvasを異なるセッションで読取 → ハッシュ不一致 | 同上 |
| R2 WebGL | `getParameter(UNMASKED_RENDERER_WEBGL)` が正規化文字列、`readPixels` はセッション毎に異なるバイト列 | unit_test + 手動 |
| R3 Grid UI | 4×3 Grid で現在開いているタブがタイル表示、ページング動作 | `tab_grid_view_unittest.cc` + 手動E2E |
| R3 Grid UI | タイルクリックで該当タブがアクティブ、Gridモードを抜ける | unit_test |
| R4 コンテキストメニュー | リンク右クリック → 「複数セッションで開く」表示 | `render_view_context_menu_browsertest.cc` (夜間) |
| R4 ダイアログ | 整数5入力+OK → 新規5タブが独立セッションで同URLを開く | 手動E2E + 統合テスト |
| R5 拡張自動ロード | pref設定済み拡張IDが新規パーティション生成時に全て自動有効化される | `partition_extension_autoloader_unittest.cc` |
| 非機能: リベース容易性 | M135→M136リベース時、`overlay_gn.patch` 以外は `git apply --3way` で自動マージ可能 | 月次リベースドキュメント |
| 非機能: ビルド再現性 | Devcontainer起動のみで、ホスト側追加セットアップなしにビルドが通る | 新環境での動作確認 |

## 7. 後続実装Specの分割

本統合Specは土台を定義するものであり、実装は以下の独立Specに分割する。

| Spec | 対象 | 依存 |
|---|---|---|
| Spec-A | Devcontainer + 骨格ビルド（空overlay + GN patch + `apply_patches.py` + CI両workflow） | — |
| Spec-B | EphemeralSessionManager + フック0001 | Spec-A |
| Spec-C | FingerprintNoiseSource + Mojo + フック0002/0003/0004 | Spec-B |
| Spec-D | MultiSessionOpenDialog + フック0005 | Spec-B |
| Spec-E | TabGridView + フック0006 + settings UI | Spec-A |
| Spec-F | PartitionExtensionAutoloader + フック0007 | Spec-B |

## 8. 設計上の確定事項まとめ

- **UI側処理は `content::BrowserThread::UI` で実行する**
- **Renderer側処理は `blink::Thread::MainThread()` で実行する**
- **スレッド跨ぎ値受け渡しは Mojo または `base::PostTask` のみを使う**
- **seed は `uint64_t` 値型のみを跨がせる**
- **IO thread へは `content/` 経由で委譲する**
- **既存Chromiumパターン（KeyedService / Supplement / Observer / `StoragePartitionConfig`）を踏襲する**
- **フックパッチは~65行、GN侵襲1パッチのみに抑える**
- **初期スコープは webdriver / Canvas / WebGL の3点、Mainスレッド上のみに限定する**
- **拡張モデルは MV3 のみ、コード共通・ストレージ分離を既定とする**
