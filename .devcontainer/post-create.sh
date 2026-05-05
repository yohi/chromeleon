#!/usr/bin/env bash
set -euo pipefail

# depot_tools の取得
if [ ! -d "${DEPOT_TOOLS_PATH}/.git" ]; then
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
      "${DEPOT_TOOLS_PATH}"
fi

# Chromium ソースの取得とタグ固定
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

# overlay 注入とパッチ適用
python3 /workspaces/custom-chromium/build/sync_overlay.py
python3 /workspaces/custom-chromium/build/apply_patches.py
