// chromium_src/overlay/chrome/browser/multi_session/fingerprint_seed_delivery.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_FINGERPRINT_SEED_DELIVERY_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_FINGERPRINT_SEED_DELIVERY_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace multi_session {

// WebContents ごとに RenderFrame 生成を監視し、
// EphemeralSessionManager から seed を取得して Renderer へ Mojo で配信する。
class FingerprintSeedDelivery
    : public content::WebContentsObserver,
      public content::WebContentsUserData<FingerprintSeedDelivery> {
public:
  ~FingerprintSeedDelivery() override;

  // content::WebContentsObserver:
  void RenderFrameCreated(content::RenderFrameHost *rfh) override;

private:
  friend class content::WebContentsUserData<FingerprintSeedDelivery>;

  explicit FingerprintSeedDelivery(content::WebContents *wc);

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

} // namespace multi_session

#endif // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_FINGERPRINT_SEED_DELIVERY_H_
