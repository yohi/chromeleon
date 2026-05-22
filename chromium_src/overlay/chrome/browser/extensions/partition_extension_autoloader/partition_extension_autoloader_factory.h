// chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader_factory.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_EXTENSIONS_PARTITION_EXTENSION_AUTOLOADER_PARTITION_EXTENSION_AUTOLOADER_FACTORY_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_EXTENSIONS_PARTITION_EXTENSION_AUTOLOADER_PARTITION_EXTENSION_AUTOLOADER_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace extensions {

class PartitionExtensionAutoloader;

class PartitionExtensionAutoloaderFactory
    : public BrowserContextKeyedServiceFactory {
public:
  static PartitionExtensionAutoloader *GetForProfile(Profile *profile);
  static PartitionExtensionAutoloaderFactory *GetInstance();

private:
  friend class base::NoDestructor<PartitionExtensionAutoloaderFactory>;

  PartitionExtensionAutoloaderFactory();
  ~PartitionExtensionAutoloaderFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext *context) const override;
  void
  RegisterProfilePrefs(user_prefs::PrefRegistrySyncable *registry) override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

} // namespace extensions

#endif // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_EXTENSIONS_PARTITION_EXTENSION_AUTOLOADER_PARTITION_EXTENSION_AUTOLOADER_FACTORY_H_
