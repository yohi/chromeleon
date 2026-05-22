// chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader.cc
#include "chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader.h"

#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/profiles/profile.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_registry.h"

namespace extensions {

PartitionExtensionAutoloader::PartitionExtensionAutoloader(Profile *profile)
    : profile_(profile) {
  auto *session_manager =
      multi_session::EphemeralSessionManagerFactory::GetForProfile(profile_);
  if (session_manager) {
    session_manager->AddObserver(this);
  }
}

PartitionExtensionAutoloader::~PartitionExtensionAutoloader() {
  auto *session_manager =
      multi_session::EphemeralSessionManagerFactory::GetForProfile(profile_);
  if (session_manager) {
    session_manager->RemoveObserver(this);
  }
}

void PartitionExtensionAutoloader::OnPartitionCreated(
    const multi_session::SessionHandle &handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const auto ids = GetAutoEnabledExtensionIds();
  auto *system = extensions::ExtensionSystem::Get(profile_);
  if (!system)
    return;
  auto *service = system->extension_service();
  if (!service)
    return;
  auto *registry = extensions::ExtensionRegistry::Get(profile_);
  if (!registry)
    return;

  for (const auto &id : ids) {
    const auto *ext = registry->GetInstalledExtension(id);
    if (!ext)
      continue;
    service->EnableExtensionForPartition(id, handle.partition_id);
  }
}

std::vector<std::string>
PartitionExtensionAutoloader::GetAutoEnabledExtensionIds() const {
  std::vector<std::string> ids;
  const PrefService *prefs = profile_->GetPrefs();
  if (!prefs)
    return ids;

  const base::Value::List &list = prefs->GetList(kAutoEnabledExtensionIds);
  for (const auto &value : list) {
    if (value.is_string()) {
      ids.push_back(value.GetString());
    }
  }
  return ids;
}

} // namespace extensions
