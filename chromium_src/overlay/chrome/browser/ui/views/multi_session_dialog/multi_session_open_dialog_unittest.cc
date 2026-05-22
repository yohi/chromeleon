// chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/multi_session_open_dialog_unittest.cc
#include "chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/multi_session_open_dialog.h"

#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.h"
#include "ui/views/controls/textfield/textfield.h"
#include "url/gurl.h"

namespace multi_session {
namespace {

class MultiSessionOpenDialogTest : public BrowserWithTestWindowTest {
 protected:
  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();
    EphemeralSessionManagerFactory::GetInstance()->SetTestingFactory(
        profile(),
        base::BindRepeating([](content::BrowserContext* context)
                                -> std::unique_ptr<KeyedService> {
          return std::make_unique<EphemeralSessionManager>(
              Profile::FromBrowserContext(context));
        }));
  }
};

TEST_F(MultiSessionOpenDialogTest, BasicTitle) {
  GURL test_url("https://example.com");
  auto dialog = std::make_unique<MultiSessionOpenDialog>(browser(), test_url);
  EXPECT_EQ(dialog->GetWindowTitle(), u"Open in Multiple Sessions");
}

}  // namespace
}  // namespace multi_session
