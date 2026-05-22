# 🦎 Chromeleon (クロメレオン)

![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=c%2B%2B&logoColor=white) 
![Chromium](https://img.shields.io/badge/Chromium-Core_Fork-blue)
![Devcontainer](https://img.shields.io/badge/Devcontainer-Ready-green)

> **ひとつのウィンドウに、無限のペルソナを。**

Chromeleonは、タブごとに完全に独立した環境を動的に生成する、Chromiumベースの次世代マルチセッション・ブラウザです。強力なアンチフィンガープリントパッチ（Blink/V8レベル）により、各タブはカメレオンのように異なる環境へシームレスに適応。Google等の厳格な認証もすり抜け、単一のブラウザ内で複数のアイデンティティを安全かつ完全に分離して運用できます。

![Demo](assets/demo.gif) <h2>🔥 Key Features</h2>
* 🛡 **True Isolation:** タブ単位での完全なCookie / Cache / IndexedDB分離（`StoragePartition`の動的生成）
* 👻 **Stealth Mode:** ハードウェアAPI、Canvas、WebGLのノイズ付与と `navigator.webdriver` のマスクによるフィンガープリント回避
* 🎛 **Native Grid UI:** C++ Viewsによる高速で直感的なタブ俯瞰システム（指定数×指定数のタイル表示とページング）
* ⚡ **Context Menu Expansion:** 右クリックから「指定した数」の独立セッションタブをバックグラウンドで一括展開

---

## 🛠 開発とビルド

### 前提条件
*   Chromium のビルド環境が整っていること（[Building Chromium](https://www.chromium.org/developers/how-tos/get-the-code/) を参照）
*   `config/gn_args.gn` の設定を確認してください。

### ビルドの高速化 (ccache)
ビルドを高速化するために `ccache` の使用を推奨します。使用する場合は、`config/gn_args.gn` に以下を設定するか、ビルド時に引数として渡してください。

```gn
cc_wrapper = "ccache"
```

> **Note:** `ccache` がインストールされていない環境で上記を設定すると、ビルドが失敗します。デフォルトでは `cc_wrapper = ""` に設定されています。
