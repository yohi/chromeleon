// chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_EXTENSIONS_PARTITION_EXTENSION_AUTOLOADER_PARTITION_EXTENSION_AUTOLOADER_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_EXTENSIONS_PARTITION_EXTENSION_AUTOLOADER_PARTITION_EXTENSION_AUTOLOADER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace extensions {

class PartitionExtensionAutoloader
    : public KeyedService,
      public multi_session::EphemeralSessionManager::Observer {
public:
  static inline constexpr char kAutoEnabledExtensionIds[] =
      "extensions.auto_enabled_partition_extension_ids";

  explicit PartitionExtensionAutoloader(Profile *profile);
  PartitionExtensionAutoloader(const PartitionExtensionAutoloader &) = delete;
  PartitionExtensionAutoloader &
  operator=(const PartitionExtensionAutoloader &) = delete;
  ~PartitionExtensionAutoloader() override;

  // EphemeralSessionManager::Observer:
  void OnPartitionCreated(const multi_session::SessionHandle &handle) override;

private:
  std::vector<std::string> GetAutoEnabledExtensionIds() const;

  raw_ptr<Profile> profile_;
};

} // namespace extensions

#endif // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_EXTENSIONS_PARTITION_EXTENSION_AUTOLOADER_PARTITION_EXTENSION_AUTOLOADER_H_
