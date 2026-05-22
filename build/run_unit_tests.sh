#!/usr/bin/env bash
# overlay モジュールのユニットテストを実行する。
# Devcontainer 内または CI の self-hosted runner 上で実行する。
set -euo pipefail

CHROMIUM_OUT="${CHROMIUM_OUT:-/workspaces/chromium/out/Default}"

echo "=== Running Chromeleon overlay unit tests ==="

# テストバイナリが存在する場合のみ実行
TEST_BINARIES=(
  "ephemeral_session_manager_unittest"
  "fingerprint_noise_source_unittest"
  "tab_grid_view_unittest"
  "multi_session_dialog_unittest"
  "partition_extension_autoloader_unittest"
)

FAILURES=0
for binary in "${TEST_BINARIES[@]}"; do
  if [ -f "${CHROMIUM_OUT}/${binary}" ]; then
    echo "--- ${binary} ---"
    if ! "${CHROMIUM_OUT}/${binary}"; then
      echo "FAILED: ${binary}"
      FAILURES=$((FAILURES + 1))
    fi
  else
    echo "SKIP: ${binary} (not built yet)"
  fi
done

echo "=== Results: ${FAILURES} failure(s) ==="
exit "${FAILURES}"
