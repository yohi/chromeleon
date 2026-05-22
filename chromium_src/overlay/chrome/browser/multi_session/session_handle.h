// chromium_src/overlay/chrome/browser/multi_session/session_handle.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_SESSION_HANDLE_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_SESSION_HANDLE_H_

#include <string>

#include "content/public/browser/storage_partition_config.h"

namespace multi_session {

// SessionHandle は StoragePartition 本体ポインタを保持しない。
// 本体が必要な呼出側は profile_->GetStoragePartition(handle.config) で
// 都度解決する。これにより BrowserContext 破棄順序や明示的 partition
// 解放経路での dangling pointer リスクを排除する。
struct SessionHandle {
  std::string partition_id; // config.partition_name と等しい
  uint64_t fingerprint_seed;
  content::StoragePartitionConfig config;
};

} // namespace multi_session

#endif // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_SESSION_HANDLE_H_
