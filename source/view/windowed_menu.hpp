#pragma once

#include "calendar_component.hpp"
#include "ftxui/util/ref.hpp"

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace caps_log::view {

using namespace ftxui;

class WindowedMenu : public ComponentBase {
    int m_selected = 0;

  public:
    WindowedMenu(std::string title, MenuOption option) {
        // TODO: messy, callers can specify their own 'selected' but get overriden here?
        option.selected = &m_selected;
        auto menuComponent = Menu(std::move(option));
        auto menuRenderer =
            Renderer(menuComponent, [title = std::move(title), menu = menuComponent]() {
                auto windowElement =
                    window(text(title), menu->Render() | frame) | size(WIDTH, LESS_THAN, 25);
                if (not menu->Focused()) {
                    windowElement |= dim;
                }
                return windowElement;
            });
        Add(menuRenderer);
    }

    auto &selected() { return m_selected; }

    static auto make(std::string title, MenuOption option) {
        return std::make_shared<WindowedMenu>(std::move(title), std::move(option));
    }
};

} // namespace caps_log::view
