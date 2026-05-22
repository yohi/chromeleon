// chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/multi_session_open_dialog.h
#ifndef CHROMIUM_SRC_OVERLAY_MULTI_SESSION_DIALOG_MULTI_SESSION_OPEN_DIALOG_H_
#define CHROMIUM_SRC_OVERLAY_MULTI_SESSION_DIALOG_MULTI_SESSION_OPEN_DIALOG_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/window/dialog_delegate.h"
#include "url/gurl.h"

class Browser;
namespace views { class Textfield; }

namespace multi_session {

class MultiSessionOpenDialog : public views::DialogDelegateView {
 public:
  static void Show(Browser* browser, const GURL& link_url);

  MultiSessionOpenDialog(Browser* browser, const GURL& link_url);
  ~MultiSessionOpenDialog() override;

  // views::DialogDelegateView:
  std::u16string GetWindowTitle() const override;
  bool Accept() override;

 private:
  raw_ptr<Browser> browser_;
  GURL link_url_;
  raw_ptr<views::Textfield> count_field_;
};

}  // namespace multi_session

#endif
