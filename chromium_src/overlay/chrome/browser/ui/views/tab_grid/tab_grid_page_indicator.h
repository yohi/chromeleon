// chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_page_indicator.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_PAGE_INDICATOR_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_PAGE_INDICATOR_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"

namespace views {
class Button;
class Label;
}

namespace multi_session {

class TabGridPageIndicator : public views::View {
 public:
  explicit TabGridPageIndicator(base::RepeatingCallback<void(int)> page_change_callback);
  TabGridPageIndicator(const TabGridPageIndicator&) = delete;
  TabGridPageIndicator& operator=(const TabGridPageIndicator&) = delete;
  ~TabGridPageIndicator() override;

  void SetPageInfo(int current_page, int total_pages);

 private:
  void OnPrevButtonClicked();
  void OnNextButtonClicked();
  void UpdateButtonsState();

  base::RepeatingCallback<void(int)> page_change_callback_;
  raw_ptr<views::Label> label_ = nullptr;
  raw_ptr<views::Button> prev_button_ = nullptr;
  raw_ptr<views::Button> next_button_ = nullptr;
  int current_page_ = 0;
  int total_pages_ = 0;
};

}  // namespace multi_session

#endif  // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_PAGE_INDICATOR_H_
