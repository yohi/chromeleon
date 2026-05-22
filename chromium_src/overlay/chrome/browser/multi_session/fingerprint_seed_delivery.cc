// chromium_src/overlay/chrome/browser/multi_session/fingerprint_seed_delivery.cc
#include "chromium_src/overlay/chrome/browser/multi_session/fingerprint_seed_delivery.h"

#include "chrome/browser/profiles/profile.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
// TODO(Phase 3): Mojo remote include を追加
// #include "chromium_src/overlay/public/mojom/multi_session/fingerprint.mojom.h"

namespace multi_session {

WEB_CONTENTS_USER_DATA_KEY_IMPL(FingerprintSeedDelivery);

FingerprintSeedDelivery::FingerprintSeedDelivery(
    content::WebContents* wc)
    : content::WebContentsObserver(wc) {}

FingerprintSeedDelivery::~FingerprintSeedDelivery() = default;

void FingerprintSeedDelivery::RenderFrameCreated(
    content::RenderFrameHost* rfh) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* mgr = EphemeralSessionManagerFactory::GetForProfile(
      Profile::FromBrowserContext(rfh->GetBrowserContext()));
  // Factory 不変条件により全 Profile 種別でサービスが存在する。
  CHECK(mgr);

  // StoragePartition 本体ポインタは保持せず、config 値のみを Manager に渡す。
  const content::StoragePartitionConfig& config =
      rfh->GetStoragePartition()->GetConfig();
  const std::optional<uint64_t> seed =
      mgr->GetSeedForPartitionConfig(config);
  if (!seed.has_value())
    return;  // 非エフェメラル（既定 Profile）は対象外

  // TODO(Phase 3): Mojo で Renderer に seed を送信
  // mojo::AssociatedRemote<blink::mojom::FingerprintSeedReceiver> remote;
  // rfh->GetRemoteAssociatedInterfaces()->GetInterface(&remote);
  // remote->SetSeed(*seed);
}

}  // namespace multi_session
