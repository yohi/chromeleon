// chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_page_indicator.cc
#include "chromium_src/overlay/chrome/browser/ui/views/tab_grid/tab_grid_page_indicator.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

namespace multi_session {

TabGridPageIndicator::TabGridPageIndicator(
    base::RepeatingCallback<void(int)> page_change_callback)
    : page_change_callback_(std::move(page_change_callback)) {
  auto *layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(8), 12));
  layout->set_main_axis_alignment(views::BoxLayout::MainAxisAlignment::kCenter);
  layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  prev_button_ = AddChildView(views::MdTextButton::Create(
      base::BindRepeating(&TabGridPageIndicator::OnPrevButtonClicked,
                          base::Unretained(this)),
      u"◀"));

  label_ = AddChildView(std::make_unique<views::Label>(u"Page 0 of 0"));

  next_button_ = AddChildView(views::MdTextButton::Create(
      base::BindRepeating(&TabGridPageIndicator::OnNextButtonClicked,
                          base::Unretained(this)),
      u"▶"));

  UpdateButtonsState();
}

TabGridPageIndicator::~TabGridPageIndicator() = default;

void TabGridPageIndicator::SetPageInfo(int current_page, int total_pages) {
  current_page_ = current_page;
  total_pages_ = total_pages;

  std::string info = base::StringPrintf(
      "Page %d of %d", total_pages_ > 0 ? current_page_ + 1 : 0, total_pages_);
  label_->SetText(base::ASCIIToUTF16(info));

  UpdateButtonsState();
}

void TabGridPageIndicator::OnPrevButtonClicked() {
  if (current_page_ > 0) {
    page_change_callback_.Run(-1);
  }
}

void TabGridPageIndicator::OnNextButtonClicked() {
  if (current_page_ < total_pages_ - 1) {
    page_change_callback_.Run(1);
  }
}

void TabGridPageIndicator::UpdateButtonsState() {
  prev_button_->SetEnabled(current_page_ > 0);
  next_button_->SetEnabled(current_page_ < total_pages_ - 1);
}

} // namespace multi_session
