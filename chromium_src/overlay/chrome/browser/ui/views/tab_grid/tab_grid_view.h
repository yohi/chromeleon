// chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_view.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_VIEW_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/views/view.h"

class BrowserView;
class TabStripModel;

namespace multi_session {

class TabGridTile;
class TabGridPageIndicator;

class TabGridView : public views::View,
                    public TabStripModelObserver {
 public:
  explicit TabGridView(BrowserView* browser_view);
  TabGridView(const TabGridView&) = delete;
  TabGridView& operator=(const TabGridView&) = delete;
  ~TabGridView() override;

  void ToggleVisibility();

  // views::View:
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(const views::SizeBounds& bounds) const override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& sel) override;

 private:
  void BuildPage(int page_index);
  void OnTileClicked(int tab_index);
  void OnPageButtonClicked(int delta);

  raw_ptr<BrowserView> browser_view_;
  raw_ptr<TabGridPageIndicator> page_indicator_ = nullptr;
  raw_ptr<views::View> tiles_container_ = nullptr;
  std::vector<raw_ptr<TabGridTile>> tiles_;

  int rows_ = 4;
  int cols_ = 3;
  int current_page_ = 0;
};

}  // namespace multi_session

#endif  // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_VIEW_H_
