#pragma once

#include <ftxui/component/component.hpp>
#include <sstream>

namespace clog::view {

class Preview : public ftxui::ComponentBase {
    int m_topLineIndex = 0;
    ftxui::Elements m_lines;

  public:
    ftxui::Element Render() override;
    bool Focusable() const override;
    bool OnEvent(ftxui::Event e) override;

    void resetScroll();
    void setContent(const std::string &str);
};

} // namespace clog::view
