#include "preview.hpp"
#include <ftxui/component/event.hpp>
#include <sstream>

namespace caps_log::view {
using namespace ftxui;

Element Preview::OnRender() {
    Elements visibleLines;
    visibleLines.push_back(m_title);
    for (int i = m_topLineIndex; i < m_lines.size(); i++) {
        visibleLines.push_back(m_lines[i]);
    }

    constexpr auto kHeight = 14;
    // Not using a `window` because of https://github.com/ArthurSonzogni/FTXUI/issues/1016
    auto element = vbox(visibleLines) | borderRounded | size(HEIGHT, EQUAL, kHeight);
    element->ComputeRequirement();
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
    m_title = text(title) | underlined | center | bold;
    Elements lines;
    std::istringstream input{str};
    for (std::string line; std::getline(input, line);) {
        lines.push_back(text(line));
    }
    m_lines = std::move(lines);
}

} // namespace caps_log::view
