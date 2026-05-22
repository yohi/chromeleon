// chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_tile.cc
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_tile.h"

#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

namespace multi_session {

TabGridTile::TabGridTile(int tab_index,
                         const std::u16string& title,
                         bool is_active,
                         PressedCallback callback)
    : views::Button(std::move(callback)),
      tab_index_(tab_index),
      is_active_(is_active) {
  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(12), 8));
  layout->set_main_axis_alignment(views::BoxLayout::MainAxisAlignment::kCenter);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto label = std::make_unique<views::Label>(title);
  label->SetMultiLine(true);
  label->SetMaxLines(2);
  label->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  title_label_ = AddChildView(std::move(label));

  SetFocusBehavior(FocusBehavior::ALWAYS);

  UpdateBorder();
}

TabGridTile::~TabGridTile() = default;

void TabGridTile::OnThemeChanged() {
  views::Button::OnThemeChanged();
  UpdateBorder();
}

void TabGridTile::UpdateBorder() {
  SkColor border_color = is_active_ ? SkColorSetRGB(26, 115, 232)    // Active: Google Blue
                                    : SkColorSetRGB(218, 220, 224);  // Inactive: Light Gray
  int thickness = is_active_ ? 3 : 1;
  SetBorder(views::CreateSolidBorder(thickness, border_color));

  SkColor bg_color = is_active_ ? SkColorSetRGB(241, 248, 255)
                                : SkColorSetRGB(255, 255, 255);
  SetBackground(views::CreateSolidBackground(bg_color));
}

}  // namespace multi_session
