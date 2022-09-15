#include "preview.hpp"

namespace clog::view {

ftxui::Element Preview::Render() {
    using namespace ftxui;
    ftxui::Elements visibleLines;
    for (int i = m_topLineIndex; i < m_lines.size(); i++) {
        visibleLines.push_back(m_lines[i]);
    }

    if (Focused())
        return vbox(visibleLines) | borderRounded | flex_shrink | size(HEIGHT, EQUAL, 14);
    else
        return vbox(visibleLines) | borderRounded | flex_shrink | size(HEIGHT, EQUAL, 14) | dim;
}

bool Preview::Focusable() const { return true; }

bool Preview::OnEvent(ftxui::Event e) {
    using namespace ftxui;
    if (!Focused()) {
        return false;
    }

    if (e == Event::ArrowDown || e == Event::Character('j')) {
        if (m_topLineIndex + 1 < m_lines.size() - 1)
            m_topLineIndex++;
        return true;
    } else if (e == Event::ArrowUp || e == Event::Character('k')) {
        if (m_topLineIndex - 1 >= 0)
            m_topLineIndex--;
        return true;
    }

    return false;
}

void Preview::resetScroll() { m_topLineIndex = 0; }

void Preview::setContent(const std::string &str) {
    using namespace ftxui;
    Elements lines;
    std::istringstream input{str};
    for (std::string line; std::getline(input, line);) {
        lines.push_back(text(line));
    }
    m_lines = std::move(lines);
}

} // namespace clog::view
