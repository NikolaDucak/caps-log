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
    ftxui::Decorator entryDecorator = nullptr;
    ftxui::Decorator selectedEntryDecorator = nullptr;
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
            [this, entryDecorator = option.entryDecorator,
             selectedEntryDecorator = option.selectedEntryDecorator](const EntryState &state) {
                std::string label = (state.active ? "> " : "  ") + state.label; // NOLINT
                auto element = text(label);
                if (state.focused || state.active) {
                    if (selectedEntryDecorator) {
                        element = element | selectedEntryDecorator;
                    } else {
                        element = element | bold | inverted;
                    }
                } else {
                    if (entryDecorator) {
                        element = element | entryDecorator;
                    } else {
                        element = element | bold;
                    }
                }
                return element;
            };

        auto menuComponent = Menu(std::move(menuOption));
        auto menuRenderer = Renderer(
            menuComponent, [title = option.title, menu = menuComponent, border = option.border]() {
                auto windowElement =
                    window(text(title), menu->Render() | vscroll_indicator | frame, border);

                if (not menu->Focused() || not menu->Active()) {
                    windowElement |= dim;
                }
                return windowElement;
            });
        Add(menuRenderer);
        menuComponent->SetActiveChild(0);
    }

    auto &selected() { return m_selected; }

    static auto make(const WindowedMenuOption &option) {
        return std::make_shared<WindowedMenu>(option);
    }

  private:
};

} // namespace caps_log::view
