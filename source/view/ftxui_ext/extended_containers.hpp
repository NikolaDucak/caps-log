#pragma once

#include "ftxui/component/component_base.hpp"

namespace caps_log::view::ftxui_ext {

/**
 * @brief A list of components, drawn in a grid and navigated * using up/down &
 * left/right arrow key or 'j'/'k' & 'h'/'l' keys.
 * @note Only width is neccesary for basic gird container functionality. It
 * represents the number of elements that are skipped when moving focus
 * verticaly
 * @note This is not supposed to be an actual FTXUI feature extension, but an
 * extension for this specific projects use.
 */
// This is a FTXUI extension so naming is kept consistent with the library
// NOLINTNEXTLINE(readability-identifier-naming);
ftxui::Component Grid(int width, ftxui::Components children, int *selected = nullptr);

/**
 * @brief A list of components, drawn in any form that can be navigated with
 * left/right arrow key or 'j'/'k' & 'h'/'l' keys, where horizontal and vertical
 * movement are treated as the same, always moving focus in one direction.
 * @note This is not supposed to be an actual FTXUI feature extension, but an
 * extension for this specific projects use.
 */
// This is a FTXUI extension so naming is kept consistent with the library
// NOLINTNEXTLINE(readability-identifier-naming);
ftxui::Component AnyDir(ftxui::Components children, int *selected = nullptr);

/**
 * @brief Same as a vertical container but it accepts custom navigation keys.
 */
// This is a FTXUI extension so naming is kept consistent with the library
// NOLINTNEXTLINE(readability-identifier-naming);
ftxui::Component CustomContainer(ftxui::Components children, ftxui::Event next, ftxui::Event prev);

} // namespace caps_log::view::ftxui_ext
