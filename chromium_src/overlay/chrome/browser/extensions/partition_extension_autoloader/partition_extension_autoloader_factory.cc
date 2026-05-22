// chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader_factory.cc
#include "chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader_factory.h"

#include "chrome/browser/extensions/extension_system_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace extensions {

// static
PartitionExtensionAutoloader *
PartitionExtensionAutoloaderFactory::GetForProfile(Profile *profile) {
  return static_cast<PartitionExtensionAutoloader *>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
PartitionExtensionAutoloaderFactory *
PartitionExtensionAutoloaderFactory::GetInstance() {
  static base::NoDestructor<PartitionExtensionAutoloaderFactory> instance;
  return instance.get();
}

PartitionExtensionAutoloaderFactory::PartitionExtensionAutoloaderFactory()
    : BrowserContextKeyedServiceFactory(
          "PartitionExtensionAutoloader",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(multi_session::EphemeralSessionManagerFactory::GetInstance());
  DependsOn(extensions::ExtensionSystemFactory::GetInstance());
}

PartitionExtensionAutoloaderFactory::~PartitionExtensionAutoloaderFactory() =
    default;

std::unique_ptr<KeyedService>
PartitionExtensionAutoloaderFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext *context) const {
  Profile *profile = Profile::FromBrowserContext(context);
  return std::make_unique<PartitionExtensionAutoloader>(profile);
}

void PartitionExtensionAutoloaderFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable *registry) {
  registry->RegisterListPref(
      PartitionExtensionAutoloader::kAutoEnabledExtensionIds,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

bool PartitionExtensionAutoloaderFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

} // namespace extensions
