// chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader_unittest.cc
#include "chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader.h"

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/test/base/testing_profile.h"
#include "chromium_src/overlay/chrome/browser/extensions/partition_extension_autoloader/partition_extension_autoloader_factory.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace {

std::vector<std::pair<std::string, std::string>> g_received_calls;

void TestCallback(const std::string &extension_id,
                  const std::string &partition_id) {
  g_received_calls.push_back({extension_id, partition_id});
}

class PartitionExtensionAutoloaderTest : public testing::Test {
protected:
  void SetUp() override {
    g_received_calls.clear();
    ExtensionService::SetEnableExtensionForPartitionCallbackForTesting(
        &TestCallback);

    TestingProfile::Builder profile_builder;
    profile_ = profile_builder.Build();

    TestExtensionSystem *system = static_cast<TestExtensionSystem *>(
        ExtensionSystem::Get(profile_.get()));
    system->CreateExtensionService(base::CommandLine::ForCurrentProcess(),
                                   base::FilePath(), false);
    extension_service_ = system->extension_service();

    multi_session::EphemeralSessionManagerFactory::GetForProfile(
        profile_.get());

    autoloader_ =
        PartitionExtensionAutoloaderFactory::GetForProfile(profile_.get());
  }

  void TearDown() override {
    ExtensionService::SetEnableExtensionForPartitionCallbackForTesting(nullptr);
    g_received_calls.clear();
  }

  void RegisterDummyExtension(const std::string &id) {
    scoped_refptr<const Extension> extension =
        ExtensionBuilder("Dummy Extension").SetID(id).Build();
    ExtensionRegistry::Get(profile_.get())->AddEnabled(extension);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<ExtensionService> extension_service_;
  raw_ptr<PartitionExtensionAutoloader> autoloader_;
};

TEST_F(PartitionExtensionAutoloaderTest,
       PrefHasInstalledExtension_EnablesOnPartitionCreated) {
  const std::string ext_id = "abcdefghijklmnopqrstuvwxyzabcdef";
  RegisterDummyExtension(ext_id);

  base::Value::List pref_list;
  pref_list.Append(ext_id);
  profile_->GetPrefs()->SetList(
      PartitionExtensionAutoloader::kAutoEnabledExtensionIds,
      std::move(pref_list));

  auto *session_manager =
      multi_session::EphemeralSessionManagerFactory::GetForProfile(
          profile_.get());
  ASSERT_TRUE(session_manager);
  const auto handle = session_manager->CreateSessionForNewTab();

  ASSERT_EQ(g_received_calls.size(), 1u);
  EXPECT_EQ(g_received_calls[0].first, ext_id);
  EXPECT_EQ(g_received_calls[0].second, handle.partition_id);
}

TEST_F(PartitionExtensionAutoloaderTest,
       PrefIsEmpty_DoesNothingOnPartitionCreated) {
  const std::string ext_id = "abcdefghijklmnopqrstuvwxyzabcdef";
  RegisterDummyExtension(ext_id);

  auto *session_manager =
      multi_session::EphemeralSessionManagerFactory::GetForProfile(
          profile_.get());
  session_manager->CreateSessionForNewTab();

  EXPECT_TRUE(g_received_calls.empty());
}

TEST_F(PartitionExtensionAutoloaderTest, ExtensionNotInstalled_SkipsEnabling) {
  const std::string ext_id = "abcdefghijklmnopqrstuvwxyzabcdef";

  base::Value::List pref_list;
  pref_list.Append(ext_id);
  profile_->GetPrefs()->SetList(
      PartitionExtensionAutoloader::kAutoEnabledExtensionIds,
      std::move(pref_list));

  auto *session_manager =
      multi_session::EphemeralSessionManagerFactory::GetForProfile(
          profile_.get());
  session_manager->CreateSessionForNewTab();

  EXPECT_TRUE(g_received_calls.empty());
}

} // namespace
} // namespace extensions
