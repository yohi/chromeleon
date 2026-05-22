// chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_unittest.cc
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager.h"

#include <set>
#include <string>

#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace multi_session {
namespace {

class EphemeralSessionManagerTest : public testing::Test {
protected:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    manager_ = std::make_unique<EphemeralSessionManager>(profile_.get());
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<EphemeralSessionManager> manager_;
};

// --- CreateSessionForNewTab テスト ---

TEST_F(EphemeralSessionManagerTest, CreateSession_ReturnsUniqueIds) {
  const auto h1 = manager_->CreateSessionForNewTab();
  const auto h2 = manager_->CreateSessionForNewTab();
  EXPECT_NE(h1.partition_id, h2.partition_id);
}

TEST_F(EphemeralSessionManagerTest, CreateSession_ReturnsUniqueSeed) {
  // 統計的に UUID v4 ベースなので衝突しない
  std::set<uint64_t> seeds;
  for (int i = 0; i < 100; ++i) {
    seeds.insert(manager_->CreateSessionForNewTab().fingerprint_seed);
  }
  // 100 回中少なくとも 95 個はユニークであることを確認
  EXPECT_GE(seeds.size(), 95u);
}

TEST_F(EphemeralSessionManagerTest, CreateSession_ConfigIsInMemory) {
  const auto h = manager_->CreateSessionForNewTab();
  EXPECT_TRUE(h.config.in_memory());
}

TEST_F(EphemeralSessionManagerTest, CreateSession_IncrementsCount) {
  EXPECT_EQ(manager_->GetSessionCountForTesting(), 0u);
  manager_->CreateSessionForNewTab();
  EXPECT_EQ(manager_->GetSessionCountForTesting(), 1u);
  manager_->CreateSessionForNewTab();
  EXPECT_EQ(manager_->GetSessionCountForTesting(), 2u);
}

// --- GetSeedForPartitionConfig テスト ---

TEST_F(EphemeralSessionManagerTest, GetSeed_RegisteredConfig_ReturnsSeed) {
  const auto h = manager_->CreateSessionForNewTab();
  const auto seed = manager_->GetSeedForPartitionConfig(h.config);
  ASSERT_TRUE(seed.has_value());
  EXPECT_EQ(*seed, h.fingerprint_seed);
}

TEST_F(EphemeralSessionManagerTest, GetSeed_UnregisteredConfig_ReturnsNullopt) {
  auto config = content::StoragePartitionConfig::Create(
      profile_.get(), "other_domain", "other_name", true);
  EXPECT_FALSE(manager_->GetSeedForPartitionConfig(config).has_value());
}

TEST_F(EphemeralSessionManagerTest, GetSeed_ZeroSeedIsValid) {
  // seed == 0 は有効な値であり、std::nullopt（未登録）とは区別される。
  const auto h = manager_->CreateSessionForNewTab();
  const auto result = manager_->GetSeedForPartitionConfig(h.config);
  ASSERT_TRUE(result.has_value());
}

// --- Observer テスト ---

class MockObserver : public EphemeralSessionManager::Observer {
public:
  int created_count = 0;
  int destroyed_count = 0;
  std::string last_created_id;
  std::string last_destroyed_id;

  void OnPartitionCreated(const SessionHandle &handle) override {
    ++created_count;
    last_created_id = handle.partition_id;
  }
  void OnPartitionDestroyed(const std::string &partition_id) override {
    ++destroyed_count;
    last_destroyed_id = partition_id;
  }
};

TEST_F(EphemeralSessionManagerTest, Observer_CreateNotifiesObserver) {
  MockObserver obs;
  manager_->AddObserver(&obs);
  const auto h = manager_->CreateSessionForNewTab();
  EXPECT_EQ(obs.created_count, 1);
  EXPECT_EQ(obs.last_created_id, h.partition_id);
  manager_->RemoveObserver(&obs);
}

} // namespace
} // namespace multi_session
