// chromium_src/overlay/chrome/browser/ui/views/tab_grid/grid_toggle_button.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_GRID_TOGGLE_BUTTON_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_GRID_TOGGLE_BUTTON_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/label_button.h"

class BrowserView;

namespace multi_session {

class GridToggleButton : public views::LabelButton {
public:
  GridToggleButton(BrowserView *browser_view, PressedCallback callback);
  GridToggleButton(const GridToggleButton &) = delete;
  GridToggleButton &operator=(const GridToggleButton &) = delete;
  ~GridToggleButton() override;

private:
  raw_ptr<BrowserView> browser_view_;
};

} // namespace multi_session

#endif // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_GRID_TOGGLE_BUTTON_H_
