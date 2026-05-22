// chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_EPHEMERAL_SESSION_MANAGER_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_EPHEMERAL_SESSION_MANAGER_H_

#include <optional>
#include <string>
#include <unordered_map>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "chromium_src/overlay/chrome/browser/multi_session/session_handle.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/storage_partition_config.h"

class Profile;
namespace content {
class StoragePartition;
class WebContents;
}  // namespace content

namespace multi_session {

class EphemeralSessionManager : public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPartitionCreated(const SessionHandle& handle) {}
    virtual void OnPartitionDestroyed(const std::string& partition_id) {}
  };

  explicit EphemeralSessionManager(Profile* profile);
  ~EphemeralSessionManager() override;
  EphemeralSessionManager(const EphemeralSessionManager&) = delete;
  EphemeralSessionManager& operator=(const EphemeralSessionManager&) = delete;

  SessionHandle CreateSessionForNewTab();
  content::StoragePartitionConfig PartitionConfigFor(
      const SessionHandle& handle) const;
  void DestroySessionForTab(content::WebContents* wc);

  // 呼出側は任意の StoragePartition* から GetConfig() を取得して渡す。
  // 非エフェメラル（既定 Profile 等）パーティションは登録されていないため
  // std::nullopt を返す。seed == 0 は正規の乱数値として有効なので、
  // sentinel ではなく std::optional で「未登録」を表現する。
  std::optional<uint64_t> GetSeedForPartitionConfig(
      const content::StoragePartitionConfig& config) const;

  void ExpandLinkInSessions(const GURL& link_url, int num_sessions);

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);

  // テスト用: 登録セッション数の取得
  size_t GetSessionCountForTesting() const { return sessions_.size(); }

 private:
  struct Entry {
    uint64_t seed;
    content::StoragePartitionConfig config;
  };
  // Key: partition_id (= config.partition_name)
  std::unordered_map<std::string, Entry> sessions_;
  raw_ptr<Profile> profile_;
  base::ObserverList<Observer> observers_;
};

}  // namespace multi_session

#endif  // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_MULTI_SESSION_EPHEMERAL_SESSION_MANAGER_H_
