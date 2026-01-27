#include "preview.hpp"

#include "markdown_text.hpp"

#include <ftxui/component/event.hpp>

namespace caps_log::view {
using namespace ftxui;

Decorator decoratorForBorderSytle(BorderStyle style) {
    using namespace ftxui;
    switch (style) {
    case BorderStyle::LIGHT:
        return borderLight;
    case BorderStyle::DASHED:
        return borderDashed;
    case BorderStyle::HEAVY:
        return borderHeavy;
    case BorderStyle::DOUBLE:
        return borderDouble;
    case BorderStyle::ROUNDED:
        return borderRounded;
    case BorderStyle::EMPTY:
        return borderEmpty;
    default:
        return border;
    }
}

Preview::Preview(const PreviewOption &option)
    : m_borderDecorator(decoratorForBorderSytle(option.border)) {}

Element Preview::OnRender() {
    Elements visibleLines;
    visibleLines.push_back(m_title);
    for (int i = m_topLineIndex; i < m_lines.size(); i++) {
        visibleLines.push_back(m_lines[i]);
    }

    // Not using a `window` because of https://github.com/ArthurSonzogni/FTXUI/issues/1016
    // Note: dont use `center` it makes the width not expanded to the full width of the screen
    auto element = vbox(visibleLines) | m_borderDecorator;
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
    m_lines = markdown(str);
}

} // namespace caps_log::view
