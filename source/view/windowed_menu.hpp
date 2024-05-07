#pragma once

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/util/ref.hpp>
#include <memory>
#include <string>

namespace caps_log::view {

struct WindowedMenuOption {
    std::string title;
    ftxui::ConstStringListRef entries;
    std::function<void()> onChange;
};

class WindowedMenu : public ftxui::ComponentBase {
    int m_selected = 0;

  public:
    WindowedMenu(const WindowedMenuOption &option) {
        using namespace ftxui;
        static constexpr auto kWidth = 25;

        MenuOption menuOption;
        menuOption.entries = option.entries;
        menuOption.on_change = option.onChange;
        menuOption.selected = &m_selected;

        auto menuComponent = Menu(std::move(menuOption));
        auto menuRenderer = Renderer(menuComponent, [title = option.title, menu = menuComponent]() {
            auto windowElement =
                window(text(title), menu->Render() | frame) | size(WIDTH, LESS_THAN, kWidth);
            if (not menu->Focused()) {
                windowElement |= dim;
            }
            return windowElement;
        });
        Add(menuRenderer);
    }

    auto &selected() { return m_selected; }

    static auto make(WindowedMenuOption option) {
        return std::make_shared<WindowedMenu>(std::move(option));
    }
};

} // namespace caps_log::view
