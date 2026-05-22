// chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/multi_session_open_dialog.cc
#include "chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/multi_session_open_dialog.h"

#include <algorithm>

#include "base/strings/string_number_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chromium_src/overlay/chrome/browser/multi_session/ephemeral_session_manager_factory.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace multi_session {

// static
void MultiSessionOpenDialog::Show(Browser *browser, const GURL &link_url) {
  auto dialog = std::make_unique<MultiSessionOpenDialog>(browser, link_url);
  views::DialogDelegate::CreateDialogWidget(
      std::move(dialog), nullptr, browser->window()->GetNativeWindow())
      ->Show();
}

MultiSessionOpenDialog::MultiSessionOpenDialog(Browser *browser,
                                               const GURL &link_url)
    : browser_(browser), link_url_(link_url) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(16), 8));

  AddChildView(std::make_unique<views::Label>(u"Number of sessions (1-20):"));

  auto field = std::make_unique<views::Textfield>();
  field->SetText(u"5");
  count_field_ = AddChildView(std::move(field));

  SetModalType(ui::MODAL_TYPE_WINDOW);
  SetButtonLabel(ui::DIALOG_BUTTON_OK, u"Open");
}

MultiSessionOpenDialog::~MultiSessionOpenDialog() = default;

std::u16string MultiSessionOpenDialog::GetWindowTitle() const {
  return u"Open in Multiple Sessions";
}

bool MultiSessionOpenDialog::Accept() {
  int n = 0;
  if (!base::StringToInt(count_field_->GetText(), &n))
    return false;
  n = std::clamp(n, 1, 20);
  auto *mgr =
      EphemeralSessionManagerFactory::GetForProfile(browser_->profile());
  CHECK(mgr);
  mgr->ExpandLinkInSessions(link_url_, n);
  return true;
}

} // namespace multi_session
