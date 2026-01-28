#pragma once

#include "markdown_text.hpp"

#include <ftxui/dom/elements.hpp>

namespace caps_log::view {

inline ftxui::Decorator identityDecorator() {
    return [](ftxui::Element element) { return element; };
}

// Shared configuration structs for view components.
struct MenuConfig {
    ftxui::BorderStyle border = ftxui::BorderStyle::ROUNDED;
    ftxui::Decorator entryDecorator = identityDecorator();
    ftxui::Decorator selectedEntryDecorator = identityDecorator();
};

struct EventsListConfig {
    ftxui::BorderStyle border = ftxui::BorderStyle::ROUNDED;
};

struct TextPreviewConfig {
    ftxui::BorderStyle border = ftxui::BorderStyle::ROUNDED;
    MarkdownTheme markdownTheme = getDefaultMarkdownTheme();
};

} // namespace caps_log::view
