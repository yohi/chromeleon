// chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_EPHEMERAL_SESSION_MANAGER_FACTORY_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_EPHEMERAL_SESSION_MANAGER_FACTORY_H_

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace multi_session {

class EphemeralSessionManager;

class EphemeralSessionManagerFactory : public ProfileKeyedServiceFactory {
 public:
  static EphemeralSessionManager* GetForProfile(Profile* profile);
  static EphemeralSessionManagerFactory* GetInstance();

  EphemeralSessionManagerFactory(const EphemeralSessionManagerFactory&) =
      delete;
  EphemeralSessionManagerFactory& operator=(
      const EphemeralSessionManagerFactory&) = delete;

 private:
  EphemeralSessionManagerFactory();
  ~EphemeralSessionManagerFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace multi_session

#endif  // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_EPHEMERAL_SESSION_MANAGER_FACTORY_H_
