# PR 未解決レビュー指摘の現行ソース確認結果

調査日: 2026-05-22

対象リポジトリ: `yohi/chromeleon`

## 概要

全 PR の未解決レビューコメントを、現在のソースに照らして確認した。

- 未解決レビューコメント総数: 191 件
- 多くは過去 PR 上の未解決スレッドであり、現在ソースでは解消済みのものも含まれる
- 現在ソース上で対応が必要なものは、CI/devcontainer/build scripts、session/fingerprint、TabGrid/UI、extension autoloader に集中している

本書では、現行ソースに対する判定を以下の3分類で整理する。

- `STILL_PRESENT`: 現在ソースでも問題が残っている
- `RESOLVED_IN_CURRENT_SOURCE`: 未解決スレッドだが現在ソースでは解消済み
- `NEEDS_RUNTIME_CONFIRMATION`: ソースだけでは判断できず、実行環境または仕様意図の確認が必要

## 修正優先度付きリスト

### P0: ビルド失敗・CI失敗に直結し得るもの

| 領域 | PR / コメント | 対象 | 現在の判定 | 対応方針 |
|---|---:|---|---|---|
| Session manager | #16 / `3286466349` | `ephemeral_session_manager.h` | `STILL_PRESENT` | `GURL` の include または forward declaration を追加する |
| Session handle | #16 / `3286466445` | `session_handle.h` | `STILL_PRESENT` | `uint64_t` 用に `<cstdint>` を include する |
| TabGrid hook | #33/#35 | `patches/0006-chrome-browser-view-grid-toggle-hook.patch` | `STILL_PRESENT` | `BrowserView::ToggleTabGrid` を実装するか、存在する API に差し替える |
| Devcontainer setup | #6/#7/#8/#15 | `.devcontainer/post-create.sh` | `STILL_PRESENT` | `config/chromium_version` からコメント・空行を除いてバージョン文字列のみ取得する |
| Build wrapper | #10/#15 | `build/build_wrapper.sh` | `STILL_PRESENT` | `CHROMIUM_SRC` を `export` し、子プロセスから参照可能にする |

### P1: 実行時クラッシュ・データ破損・重大な不安定化リスク

| 領域 | PR / コメント | 対象 | 現在の判定 | 対応方針 |
|---|---:|---|---|---|
| Fingerprint seed delivery | #18/#21 | `fingerprint_seed_delivery.cc` | `STILL_PRESENT` | `CHECK(mgr)` を避け、対象外 Profile では return する |
| Fingerprint noise | #23/#25/#27 | `fingerprint_noise_source.cc` | `STILL_PRESENT` | `ImageData*` の null check と RGBA 前提の境界確認を追加する |
| Fingerprint seed | #23/#27 | `fingerprint_noise_source.cc` | `STILL_PRESENT` | `uint64_t` seed の上位 32bit を捨てない折りたたみ方式にする |
| Session manager | #17/#21 | `ephemeral_session_manager.cc` | `STILL_PRESENT` | `DestroySessionForTab()` で `StoragePartitionConfig` 全体の一致確認を行う |
| TabGrid lifecycle | #32/#35 | `tab_grid_view.cc` | `STILL_PRESENT` | `tiles_container_` の dangling raw_ptr を避け、削除後に null 化または所有構造を見直す |
| TabGrid null safety | #32/#35 | `tab_grid_view.cc` | `STILL_PRESENT` | `browser_view_` / `browser()` / `tab_strip_model()` の null check を一貫させる |
| Extension autoloader | #36/#38 | `partition_extension_autoloader.cc` | `STILL_PRESENT` | observer 解除を destructor ではなく `Shutdown()` override に移す |
| Devcontainer PATH | #6 / `3286455264` | `.devcontainer/devcontainer.json` | `STILL_PRESENT` | `containerEnv.PATH` の `${PATH}` 依存をやめ、固定値または `remoteEnv` を使う |

### P2: CI再現性・保守性・テスト品質の改善

| 領域 | PR / コメント | 対象 | 現在の判定 | 対応方針 |
|---|---:|---|---|---|
| PR lint permissions | #14 / `3286447183` | `.github/workflows/pr-lint.yml` | `STILL_PRESENT` | `permissions: contents: read` を追加する |
| Nightly concurrency | #13 / `3286447212` | `.github/workflows/nightly-build.yml` | `STILL_PRESENT` | `concurrency` を追加して同時実行を防ぐ |
| Nightly labels | #11 / `3186128541` | `.github/workflows/nightly-build.yml` | `NEEDS_RUNTIME_CONFIRMATION` | `bug` / `nightly-build` label の存在を確認、または label なしでも作成可能にする |
| Patch apply scripts | #9/#15 | `build/apply_patches.py` | `STILL_PRESENT` | `CHROMIUM_SRC` 未設定時に明示エラーを出す |
| Patch unapply scripts | #7/#9/#15 | `build/unapply_patches.py` | `STILL_PRESENT` | `CHROMIUM_SRC` 明示エラーと `--3way` 併用を検討する |
| Patch dry-run | #7/#8/#10/#15 | `build/ci_patch_dryrun.py` | `STILL_PRESENT` | `git apply --stat` ではなく `--check` を使い、`cwd=project_root` を指定する |
| Dialog UX | #28/#30 | `multi_session_open_dialog.cc` | `STILL_PRESENT` | 無効入力・クランプ時にユーザーへフィードバックを出す |
| Dialog tests | #29/#30 | `multi_session_open_dialog_unittest.cc` | `STILL_PRESENT` | `Accept()` の正常系・異常系テストを追加する |
| Dialog lifetime | #30 | `multi_session_open_dialog.cc` | `NEEDS_RUNTIME_CONFIRMATION` | `raw_ptr<Browser>` 保持が Dialog 寿命上安全か確認する |
| TabGrid layout | #32 | `tab_grid_view.cc` | `STILL_PRESENT` | `Layout()` 直接呼びを避け、Views の推奨 invalidation API を使う |
| TabGrid style | #31/#35 | `tab_grid_tile.cc` | `STILL_PRESENT` | ハードコード色をテーマ色に置き換える |
| TabGrid API | #31 | `tab_grid_page_indicator.cc` | `STILL_PRESENT` | `base::ASCIIToUTF16` の代替 API を使う |
| Extension prefs | #36/#38 | `partition_extension_autoloader_factory.cc` | `NEEDS_RUNTIME_CONFIRMATION` | `SYNCABLE_PREF` にする仕様意図を確認し、必要なら `UNSYNCABLE_PREF` にする |
| Extension tests | #37/#38 | `partition_extension_autoloader_unittest.cc` | `STILL_PRESENT` | `g_received_calls` のグローバル状態を fixture 管理に変更し、未使用 member を削除する |
| GN formatting | #37 | `partition_extension_autoloader/BUILD.gn` | `STILL_PRESENT` | ファイル末尾の余分な空行を削除する |

## 現在ソースでは解消済みの主な指摘

| 領域 | PR / コメント | 判定内容 |
|---|---:|---|
| PR lint runner | #14 / `3286446834` | `ubuntu-latest` に変更済み |
| Node setup | #14 / `3286447041` | `actions/setup-node@v4` と Node 20 が追加済み |
| Ruff version | #13/#14 | `ruff==0.4.4` に固定済み |
| Nightly GN args path | #14 / `3286446952` | `${CHROMIUM_CONFIG_DIR}/gn_args.gn` 参照に変更済み |
| Artifact retention | #14 / `3286447290` | `retention-days: 7` 設定済み |
| ccache setting | #5 | `cc_wrapper = ""` で未導入環境の失敗を回避済み |
| Storage partition patch | #20/#21 | `patches/0001-content-storage-partition-hook.patch` は現在存在せず、`series` からも外れている |
| Context menu string | #39 / `3287724152` | `IDS_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION` 定義済み |
| Extension partition hook | #37/#38 | `EnableExtensionForPartition` は stub ではなく実処理あり |
| Extension hook whitespace | #39 / `3287724314` | trailing whitespace は現在見当たらない |
| Profile keyed services hook | #39 / `3287724384` | 未使用 include 指摘は解消済み |
| Patch series comment | #39 / `3287724551` | 「全て空スタブ」コメントは削除済み。ただしコメント重複は残存 |
| TabGrid tests syntax | #34/#35 | 余分な閉じ括弧・namespace 不一致は解消済み |
| Docs local path | #2 | plan の Design Spec リンクは相対リンクになっている |

## 実行確認が必要な項目

以下はソース確認だけでは完全に判定できない。

1. `nightly-build.yml` の self-hosted runner
   - `runs-on: [self-hosted, linux, chromium-builder]` に対応する runner が GitHub 側に登録されているか確認する。
2. `nightly-build` label
   - workflow が issue 作成時に `bug` / `nightly-build` label を付けるため、ラベルが存在するか確認する。
3. Dialog の Browser lifetime
   - `MultiSessionOpenDialog` が保持する `raw_ptr<Browser>` が Dialog の寿命中に安全か、実行時に確認する。
4. Fingerprint seed delivery
   - Mojo 配線は存在するが、renderer 側に seed が届き、Canvas/WebGL noise に反映されるか Chromium 実行環境で確認する。
5. Extension autoloader の pref sync 方針
   - `kAutoEnabledExtensionIds` を sync 対象にする仕様意図を確認する。

## 推奨対応順

1. P0 の compile/build break リスクを先に解消する。
2. P1 の runtime crash / memory safety / fingerprint correctness を修正する。
3. CI の `permissions` と `concurrency` を追加する。
4. Test coverage と UI/UX/スタイル指摘をまとめて処理する。
5. 実行環境依存の項目を GitHub Actions と Chromium 実機で確認する。
