#pragma once

#include "markdown_text.hpp"

#include <ftxui/dom/elements.hpp>

namespace caps_log::view {

// Shared configuration structs for view components.
struct MenuConfig {
    ftxui::BorderStyle border = ftxui::BorderStyle::ROUNDED;
};

struct EventsListConfig {
    ftxui::BorderStyle border = ftxui::BorderStyle::ROUNDED;
};

struct TextPreviewConfig {
    ftxui::BorderStyle border = ftxui::BorderStyle::ROUNDED;
    MarkdownTheme markdownTheme = getDefaultMarkdownTheme();
};

} // namespace caps_log::view
