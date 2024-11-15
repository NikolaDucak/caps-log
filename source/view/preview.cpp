#include "preview.hpp"
#include <sstream>

namespace caps_log::view {
using namespace ftxui;

Element Preview::Render() {
    Elements visibleLines;
    for (int i = m_topLineIndex; i < m_lines.size(); i++) {
        visibleLines.push_back(m_lines[i]);
    }

    constexpr auto kHeight = 14;
    auto element = window(m_title, vbox(visibleLines)) | flex_shrink | size(HEIGHT, EQUAL, kHeight);
    return Focused() ? element : element | dim;
}

bool Preview::Focusable() const { return true; }

bool Preview::OnEvent(Event event) {
    if (!Focused()) {
        return false;
    }

    if (event == Event::ArrowDown || event == Event::Character('j')) {
        if (m_topLineIndex + 1 < m_lines.size() - 1) {
            m_topLineIndex++;
        }
        return true;
    }
    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (m_topLineIndex - 1 >= 0) {
            m_topLineIndex--;
        }
        return true;
    }

    return false;
}

void Preview::resetScroll() { m_topLineIndex = 0; }

void Preview::setContent(const std::string &title, const std::string &str) {
    m_title = text(title) | underlined | center;
    Elements lines;
    std::istringstream input{str};
    for (std::string line; std::getline(input, line);) {
        lines.push_back(text(line));
    }
    m_lines = std::move(lines);
}

} // namespace caps_log::view
