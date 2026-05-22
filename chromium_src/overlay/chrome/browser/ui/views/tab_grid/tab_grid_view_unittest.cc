// chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_view_unittest.cc
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_view.h"

#include <memory>
#include <vector>

#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/test_with_browser_view.h"
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_page_indicator.h"
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_tile.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/view.h"

namespace multi_session {

class TabGridViewTest : public TestWithBrowserView {
protected:
  void SetUp() override { TestWithBrowserView::SetUp(); }

  void AddTabs(int count) {
    for (int i = 0; i < count; ++i) {
      AddTab(browser(), GURL("about:blank"));
    }
  }
};

// 1. レイアウト計算 & ページング境界テスト
TEST_F(TabGridViewTest, PagingAndLayout) {
  // 初期状態ではタブは0枚
  auto tab_grid_view = std::make_unique<TabGridView>(browser_view());
  tab_grid_view->SetVisible(true);

  // タブを5枚追加する
  AddTabs(5);
  tab_grid_view->BuildPage(0); // 反映

  // 12枚以下の場合は1ページ目で全タイルが表示される
  EXPECT_EQ(tab_grid_view->current_page(), 0);
  EXPECT_EQ(tab_grid_view->tiles().size(), 5u);

  // さらに10枚追加（合計15枚）
  AddTabs(10);
  tab_grid_view->BuildPage(0);

  // 1ページ目の表示上限は 4 * 3 = 12枚
  EXPECT_EQ(tab_grid_view->current_page(), 0);
  EXPECT_EQ(tab_grid_view->tiles().size(), 12u);

  // 次のページへ遷移をシミュレート
  tab_grid_view->OnPageButtonClicked(1);
  EXPECT_EQ(tab_grid_view->current_page(), 1);
  // 15 - 12 = 3枚が表示されているはず
  EXPECT_EQ(tab_grid_view->tiles().size(), 3u);

  // 前のページへ戻る遷移をシミュレート
  tab_grid_view->OnPageButtonClicked(-1);
  EXPECT_EQ(tab_grid_view->current_page(), 0);
  EXPECT_EQ(tab_grid_view->tiles().size(), 12u);

  // ページングの境界テスト: 0ページ目より前へ遷移しようとした場合は0にとどまる
  tab_grid_view->OnPageButtonClicked(-1);
  EXPECT_EQ(tab_grid_view->current_page(), 0);

  // ページングの境界テスト:
  // 最大ページ数（2ページなのでインデックス1）を超えて遷移しようとした場合は最大にとどまる
  tab_grid_view->OnPageButtonClicked(2);
  EXPECT_EQ(tab_grid_view->current_page(), 1);
}

// 2. タイルクリック -> ActivateTabAt & 非表示化テスト
TEST_F(TabGridViewTest, TileClickedSelectionAndHide) {
  auto tab_grid_view = std::make_unique<TabGridView>(browser_view());
  tab_grid_view->SetVisible(true);

  AddTabs(5);
  tab_grid_view->BuildPage(0);

  TabStripModel *model = browser()->tab_strip_model();
  // 初期状態ではアクティブなインデックスは 4（最後に追加されたタブ）
  EXPECT_EQ(model->active_index(), 4);

  // インデックス 2 のタイルをクリックするのをシミュレート
  tab_grid_view->OnTileClicked(2);

  // アクティブなタブがインデックス 2 に切り替わっていることを検証
  EXPECT_EQ(model->active_index(), 2);
  // クリックされたらグリッドビューが非表示になることを検証
  EXPECT_FALSE(tab_grid_view->GetVisible());
}

// 3. 可視性トグル & 更新テスト
TEST_F(TabGridViewTest, ToggleVisibilityUpdatesPage) {
  auto tab_grid_view = std::make_unique<TabGridView>(browser_view());
  // 初期状態は非表示（コンストラクタ直後かつToggleVisibilityを呼んでいないため、デフォルトはビューのデフォルトに依存）
  tab_grid_view->SetVisible(false);

  // 非表示の状態でタブを3枚追加
  AddTabs(3);

  // トグルで表示にする
  tab_grid_view->ToggleVisibility();
  EXPECT_TRUE(tab_grid_view->GetVisible());

  // 表示された時に、最新のタブ3枚を反映してタイルが作られていることを検証
  EXPECT_EQ(tab_grid_view->tiles().size(), 3u);

  // トグルで再度非表示にする
  tab_grid_view->ToggleVisibility();
  EXPECT_FALSE(tab_grid_view->GetVisible());
}

} // namespace
} // namespace multi_session
