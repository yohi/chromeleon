// chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_tile.h
#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_TILE_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_TILE_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/controls/button/button.h"

namespace views {
class Label;
}

namespace multi_session {

class TabGridTile : public views::Button {
public:
  TabGridTile(int tab_index, const std::u16string &title, bool is_active,
              PressedCallback callback);
  TabGridTile(const TabGridTile &) = delete;
  TabGridTile &operator=(const TabGridTile &) = delete;
  ~TabGridTile() override;

  int tab_index() const { return tab_index_; }
  bool is_active() const { return is_active_; }

  // views::View:
  void OnThemeChanged() override;

private:
  void UpdateBorder();

  int tab_index_;
  bool is_active_;
  raw_ptr<views::Label> title_label_ = nullptr;
};

} // namespace multi_session

#endif // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_TAB_GRID_TAB_GRID_TILE_H_
