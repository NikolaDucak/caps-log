#pragma once

#include <ftxui/component/component.hpp>

namespace caps_log::view {

class Preview : public ftxui::ComponentBase {
    int m_topLineIndex = 0;
    ftxui::Elements m_lines;

  public:
    ftxui::Element Render() override;
    bool Focusable() const override;
    bool OnEvent(ftxui::Event event) override;

    void resetScroll();
    void setContent(const std::string &str);
};

} // namespace caps_log::view
