#pragma once

#include <sstream>
#include <ftxui/component/component.hpp>

namespace clog::view {

class Preview : public ftxui::ComponentBase {
    int m_topLineIndex = 0;
    ftxui::Elements m_lines;

public:

    ftxui::Element Render() override;
    bool Focusable() const override;
    bool OnEvent(ftxui::Event e) override;

    void setContent(const std::string& str);
};

}
