#!/usr/bin/env bash
# Chromeleon の標準ビルド手順をラップするスクリプト。
set -euo pipefail

CHROMIUM_SRC="${CHROMIUM_SRC:-/workspaces/chromium/src}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "=== Chromeleon Build Wrapper ==="
echo "Chromium src: ${CHROMIUM_SRC}"
echo "Project root: ${PROJECT_ROOT}"

# Step 1: overlay 同期
echo "--- Syncing overlay ---"
python3 "${PROJECT_ROOT}/build/sync_overlay.py"

# Step 2: パッチ適用
echo "--- Applying patches ---"
python3 "${PROJECT_ROOT}/build/apply_patches.py"

# Step 3: GN gen
echo "--- GN gen ---"
cd "${CHROMIUM_SRC}"
gn gen out/Default --args="$(cat "${PROJECT_ROOT}/config/gn_args.gn")"

# Step 4: ビルド
echo "--- Building chrome ---"
autoninja -C out/Default chrome

echo "=== Build complete ==="
echo "Run: ${CHROMIUM_SRC}/out/Default/chrome --enable-features=MultiSessionTabs --user-data-dir=/tmp/chromium-profile"
