#pragma once

#include "view/view_layout_base.hpp"
#include <chrono>

namespace caps_log::view {

struct ScratchpadData {
    std::string title;
    std::string content;
    std::chrono::year_month_day dateModified;
};

class ScratchpadViewLayoutBase : public ViewLayoutBase {
  public:
    virtual void setScratchpads(const std::vector<ScratchpadData> &ScratchpadData) = 0;
};
} // namespace caps_log::view
