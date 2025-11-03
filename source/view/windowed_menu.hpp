#pragma once

#include "view/view.hpp"
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
    ViewConfig::Logs::Menu config;

    ftxui::Decorator selectedDecorator = ftxui::inverted;
    ftxui::Decorator unselectedDecorator = ftxui::nothing;
    ftxui::BorderStyle border = ftxui::BorderStyle::ROUNDED;
};

class WindowedMenu : public ftxui::ComponentBase {
    int m_selected = 0;

  public:
    explicit WindowedMenu(const WindowedMenuOption &option) {
        using namespace ftxui;

        MenuOption menuOption;
        menuOption.entries = option.entries;
        menuOption.on_change = option.onChange;
        menuOption.selected = &m_selected;
        menuOption.entries_option.transform = [option](const EntryState &state) {
            auto element = text(state.label);
            if (state.focused) {
                element |= option.selectedDecorator;
            } else {
                element |= option.unselectedDecorator;
            }
            return element;
        };
        auto menuComponent = Menu(std::move(menuOption));
        auto menuRenderer = Renderer(menuComponent, [option, menu = menuComponent]() {
            auto windowElement = window(text(option.title),
                                        menu->Render() | vscroll_indicator | frame, option.border);

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
