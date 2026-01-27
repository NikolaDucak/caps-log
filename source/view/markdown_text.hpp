#pragma once

#include <algorithm>
#include <array>
#include <ftxui/dom/elements.hpp>
#include <string_view>

namespace caps_log::view {

struct MarkdownTheme {
    // NOLINTNEXTLINE(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
    std::array<ftxui::Color, 6> headerShades = {
        ftxui::Color::Palette256::Yellow4,         // H1 - lightest
        ftxui::Color::Palette256::DarkOliveGreen3, // H2
        ftxui::Color::Palette256::DarkSeaGreen,    // H3
        ftxui::Color::Palette256::SkyBlue3,        // H4
        ftxui::Color::Palette256::SkyBlue1,        // H5
        ftxui::Color::Palette256::LightSkyBlue3,   // H6 - darkest
    };
    ftxui::Color list = ftxui::Color::Blue;
    ftxui::Color quote = ftxui::Color::Purple;
    ftxui::Color codeFg = ftxui::Color::GrayLight;

    [[nodiscard]] ftxui::Color headerColorForLevel(std::size_t headerLevel) const {
        headerLevel = std::max(headerLevel, std::size_t{1});
        headerLevel = std::min(headerLevel, headerShades.size());
        return headerShades.at(headerLevel - 1);
    }
};

const MarkdownTheme &getDefaultMarkdownTheme();

ftxui::Elements markdown(std::string_view documentView,
                         const MarkdownTheme &theme = getDefaultMarkdownTheme());

} // namespace caps_log::view
