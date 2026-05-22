// chromium_src/overlay/chrome/browser/ui/views/tab_grid/grid_toggle_button.cc
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/grid_toggle_button.h"

#include "chrome/browser/ui/views/frame/browser_view.h"

namespace multi_session {

GridToggleButton::GridToggleButton(BrowserView *browser_view,
                                   PressedCallback callback)
    : views::LabelButton(std::move(callback), u"Grid"),
      browser_view_(browser_view) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetStyle(views::Button::ButtonStyle::kText);
}

GridToggleButton::~GridToggleButton() = default;

} // namespace multi_session
