#pragma once

#include "ftxui/component/component_base.hpp"

namespace clog::view::ftxui_ext {
using namespace ftxui;

/**
 * @brief A list of components, drawn in a grid and navigated * using up/down &
 * left/right arrow key or 'j'/'k' & 'h'/'l' keys.
 * @note Only width is neccesary for basic gird container functionality. It
 * represents the number of elements are skipped when moving focus verticaly
 * @note This is not supposed to be an actual FTXUI feature extension, but an
 * extension for this specific projects use.
 */
Component Grid(int width, Components children);

/**
 * @brief A list of components, drawn in any form that can be navigated with
 * left/right arrow key or 'j'/'k' & 'h'/'l' keys, where horizontal and vertical
 * movement are treated as the same, always moving focus in one direction.
 * @note This is not supposed to be an actual FTXUI feature extension, but an
 * extension for this specific projects use.
 */
Component AnyDir(Components children);

/**
 * @brief Same as a vertical container but it accepts custom navigation keys.
 */
Component CustomContainer(Components children, Event next, Event prev);

}  // namespace clog::view::ftxui_ext
