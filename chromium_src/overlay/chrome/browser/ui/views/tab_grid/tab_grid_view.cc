// chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_view.cc
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_view.h"

#include <algorithm>
#include <memory>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_page_indicator.h"
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_tile.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/table_layout.h"

namespace multi_session {

TabGridView::TabGridView(BrowserView* browser_view)
    : browser_view_(browser_view) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(16), 16));

  auto indicator = std::make_unique<TabGridPageIndicator>(
      base::BindRepeating(&TabGridView::OnPageButtonClicked,
                          base::Unretained(this)));
  page_indicator_ = AddChildView(std::move(indicator));

  if (browser_view_ && browser_view_->browser()) {
    browser_view_->browser()->tab_strip_model()->AddObserver(this);
  }

  BuildPage(0);
}

TabGridView::~TabGridView() {
  if (browser_view_ && browser_view_->browser()) {
    browser_view_->browser()->tab_strip_model()->RemoveObserver(this);
  }
}

void TabGridView::ToggleVisibility() {
  SetVisible(!GetVisible());
  if (GetVisible()) {
    BuildPage(current_page_);
  }
}

void TabGridView::Layout(PassKey pass_key) {
  views::View::Layout(pass_key);
}

gfx::Size TabGridView::CalculatePreferredSize(const views::SizeBounds& bounds) const {
  return views::View::CalculatePreferredSize(bounds);
}

void TabGridView::OnTabStripModelChanged(
    TabStripModel* model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& sel) {
  if (GetVisible()) {
    BuildPage(current_page_);
  }
}

void TabGridView::BuildPage(int page_index) {
  tiles_.clear();

  TabStripModel* model = browser_view_->browser()->tab_strip_model();
  int total_tabs = model->count();
  int page_size = rows_ * cols_;
  int total_pages = (total_tabs + page_size - 1) / page_size;
  if (total_pages == 0) total_pages = 1;

  if (page_index >= total_pages) {
    page_index = total_pages - 1;
  }
  if (page_index < 0) {
    page_index = 0;
  }
  current_page_ = page_index;

  page_indicator_->SetPageInfo(current_page_, total_pages);

  if (tiles_container_) {
    RemoveChildViewT(tiles_container_.get());
  }

  auto container = std::make_unique<views::View>();
  auto* layout = container->SetLayoutManager(std::make_unique<views::TableLayout>());
  for (int i = 0; i < cols_; ++i) {
    layout->AddColumn(views::LayoutAlignment::kFill, views::LayoutAlignment::kFill,
                      1.0f, views::TableLayout::ColumnSize::kUsePreferred, 0, 0);
  }
  for (int i = 0; i < rows_; ++i) {
    layout->AddRows(1, 1.0f);
  }

  int start_tab = current_page_ * page_size;
  int active_index = model->active_index();

  for (int i = 0; i < page_size; ++i) {
    int tab_idx = start_tab + i;
    if (tab_idx < total_tabs) {
      content::WebContents* web_contents = model->GetWebContentsAt(tab_idx);
      std::u16string title = web_contents->GetTitle();
      if (title.empty()) {
        title = u"Untitled";
      }
      bool is_active = (tab_idx == active_index);

      auto tile = std::make_unique<TabGridTile>(
          tab_idx, title, is_active,
          base::BindRepeating(&TabGridView::OnTileClicked,
                              base::Unretained(this), tab_idx));
      tiles_.push_back(container->AddChildView(std::move(tile)));
    } else {
      container->AddChildView(std::make_unique<views::View>());
    }
  }

  // page_indicator_ の上 (0番目の位置) に挿入
  tiles_container_ = AddChildViewAt(std::move(container), 0);
  Layout();
}

void TabGridView::OnTileClicked(int tab_index) {
  TabStripModel* model = browser_view_->browser()->tab_strip_model();
  if (tab_index >= 0 && tab_index < model->count()) {
    model->ActivateTabAt(
        tab_index,
        TabStripUserGestureDetails(
            TabStripUserGestureDetails::GestureType::kOther));
    SetVisible(false);
  }
}

void TabGridView::OnPageButtonClicked(int delta) {
  BuildPage(current_page_ + delta);
}

}  // namespace multi_session
