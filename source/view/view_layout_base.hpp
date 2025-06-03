#pragma once

#include <ftxui/component/component_base.hpp>
namespace caps_log::view {

class ViewLayoutBase {
  public:
    ViewLayoutBase() = default;
    ViewLayoutBase(const ViewLayoutBase &) = default;
    ViewLayoutBase(ViewLayoutBase &&) = default;
    ViewLayoutBase &operator=(const ViewLayoutBase &) = default;
    ViewLayoutBase &operator=(ViewLayoutBase &&) = default;

    virtual ~ViewLayoutBase() = default;

    virtual ftxui::Component getComponent() = 0;
};
} // namespace caps_log::view
