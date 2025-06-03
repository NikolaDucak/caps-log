#include "preview.hpp"
#include <ftxui/component/event.hpp>
#include <regex>
#include <sstream>

namespace caps_log::view {
using namespace ftxui;

/**
 * @brief Decorates a markdown line with appropriate styling.
 * Headers are bolded and purple, code blocks are gray, lists are bllue, and paragraphs are normal.
 */
Element decorateMarkdownLine(const std::string &line) {
    if (line.starts_with("# ") || line.starts_with("## ") || line.starts_with("### ")) {
        return text(line) | bold | color(Color::Green);
    }
    if (line.starts_with("> ")) {
        return text(line) | color(Color::Purple); // Blockquote
    }
    if (line.starts_with("- ") || line.starts_with("* ")) {
        return text(line) | color(Color::Blue); // List item
    }
    // check if line starts with ordered list with regex
    if (std::regex_search(line, std::regex("^\\d+\\. "))) {
        return text(line) | color(Color::Blue); // Ordered list item
    }

    return text(line); // Normal paragraph
}

Element Preview::OnRender() {
    Elements visibleLines;
    visibleLines.push_back(m_title);
    for (int i = m_topLineIndex; i < m_lines.size(); i++) {
        visibleLines.push_back(m_lines[i]);
    }

    // Not using a `window` because of https://github.com/ArthurSonzogni/FTXUI/issues/1016
    // Note: dont use `center` it makes the width not expanded to the full width of the screen
    auto element = vbox(visibleLines) | borderRounded;
    return (Focused() ? element : element | dim);
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
        lines.push_back(decorateMarkdownLine(line));
    }
    m_lines = std::move(lines);
}

} // namespace caps_log::view
