// chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.cc
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.h"

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager.h"

namespace multi_session {

// static
EphemeralSessionManager* EphemeralSessionManagerFactory::GetForProfile(
    Profile* profile) {
  return static_cast<EphemeralSessionManager*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

// static
EphemeralSessionManagerFactory*
EphemeralSessionManagerFactory::GetInstance() {
  static base::NoDestructor<EphemeralSessionManagerFactory> instance;
  return instance.get();
}

EphemeralSessionManagerFactory::EphemeralSessionManagerFactory()
    : ProfileKeyedServiceFactory(
          "EphemeralSessionManager",
          ProfileSelections::BuildForRegularAndIncognito()) {}

EphemeralSessionManagerFactory::~EphemeralSessionManagerFactory() = default;

std::unique_ptr<KeyedService>
EphemeralSessionManagerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<EphemeralSessionManager>(
      Profile::FromBrowserContext(context));
}

}  // namespace multi_session
