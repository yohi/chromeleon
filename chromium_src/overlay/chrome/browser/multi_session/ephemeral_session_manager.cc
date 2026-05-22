// chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager.cc
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager.h"

#include "base/rand_util.h"
#include "base/uuid.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"

namespace multi_session {

EphemeralSessionManager::EphemeralSessionManager(Profile* profile)
    : profile_(profile) {}

EphemeralSessionManager::~EphemeralSessionManager() = default;

SessionHandle EphemeralSessionManager::CreateSessionForNewTab() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const std::string pid =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  const uint64_t seed = base::RandUint64();

  auto config = content::StoragePartitionConfig::Create(
      profile_, /*partition_domain=*/"multi_session",
      /*partition_name=*/pid, /*in_memory=*/true);

  // 実体を生成するためだけに呼ぶ。返り値はあえて保持しない。
  profile_->GetStoragePartition(config, /*can_create=*/true);

  sessions_.emplace(pid, Entry{seed, config});
  SessionHandle h{pid, seed, config};
  for (auto& obs : observers_)
    obs.OnPartitionCreated(h);
  return h;
}

content::StoragePartitionConfig EphemeralSessionManager::PartitionConfigFor(
    const SessionHandle& handle) const {
  return handle.config;
}

void EphemeralSessionManager::DestroySessionForTab(
    content::WebContents* wc) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!wc)
    return;

  const auto& config =
      wc->GetBrowserContext()->GetStoragePartition(
          wc->GetSiteInstance()->GetStoragePartitionConfig())
      ->GetConfig();

  const auto it = sessions_.find(config.partition_name());
  if (it == sessions_.end())
    return;

  const std::string partition_id = it->first;
  sessions_.erase(it);
  for (auto& obs : observers_)
    obs.OnPartitionDestroyed(partition_id);
}

std::optional<uint64_t>
EphemeralSessionManager::GetSeedForPartitionConfig(
    const content::StoragePartitionConfig& config) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const auto it = sessions_.find(config.partition_name());
  if (it == sessions_.end())
    return std::nullopt;
  // partition_name が偶然衝突した場合に備え、config 全体で一致確認を行う。
  if (it->second.config != config)
    return std::nullopt;
  return it->second.seed;
}

void EphemeralSessionManager::ExpandLinkInSessions(
    const GURL& url, int n) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (int i = 0; i < n; ++i) {
    SessionHandle h = CreateSessionForNewTab();
    NavigateParams params(profile_, url, ui::PAGE_TRANSITION_LINK);
    params.disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
    params.storage_partition_config = PartitionConfigFor(h);
    Navigate(&params);
  }
}

void EphemeralSessionManager::AddObserver(Observer* obs) {
  observers_.AddObserver(obs);
}

void EphemeralSessionManager::RemoveObserver(Observer* obs) {
  observers_.RemoveObserver(obs);
}

}  // namespace multi_session
