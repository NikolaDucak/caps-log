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
    ftxui::BorderStyle border;
    ftxui::Decorator entryDecorator;
    ftxui::Decorator selectedEntryDecorator;
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
        menuOption.entries_option.transform =
            [entryDecorator = option.entryDecorator,
             selectedEntryDecorator = option.selectedEntryDecorator](const EntryState &state) {
                auto element = text(state.label);
                if (state.active) {
                    element = element | selectedEntryDecorator;
                } else {
                    element = element | entryDecorator;
                }
                return element;
            };

        auto menuComponent = Menu(std::move(menuOption));
        auto menuRenderer = Renderer(
            menuComponent, [title = option.title, menu = menuComponent, border = option.border]() {
                auto windowElement =
                    window(text(title), menu->Render() | vscroll_indicator | frame, border);

                if (not menu->Focused()) {
                    windowElement |= dim;
                }
                return windowElement;
            });
        Add(menuRenderer);
    }

    auto &selected() { return m_selected; }

    static auto make(const WindowedMenuOption &option) {
        return std::make_shared<WindowedMenu>(option);
    }
};

} // namespace caps_log::view
