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
    struct Look {
        bool border = true;
        ftxui::Color color = ftxui::Color::Default;
        ViewConfig::StyleMask style{};
        ftxui::Color selected_color = ftxui::Color::Default;
        ViewConfig::StyleMask selected_style{};
    } look{};

    ftxui::Decorator selectedDecorator = ftxui::inverted;
    ftxui::Decorator unselectedDecorator = ftxui::nothing;
};

class WindowedMenu : public ftxui::ComponentBase {
    int m_selected = 0;

  public:
    explicit WindowedMenu(const WindowedMenuOption &option) {
        using namespace ftxui;

        auto applyStyle = [](Element el, const ViewConfig::StyleMask &style) {
            if (style.bold) {
                el |= bold;
            }
            if (style.underline) {
                el |= underlined;
            }
            if (style.italic) {
                el |= italic;
            }
            return el;
        };

        MenuOption menuOption;
        menuOption.entries = option.entries;
        menuOption.on_change = option.onChange;
        menuOption.selected = &m_selected;
        menuOption.entries_option.transform = [option, applyStyle](const EntryState &state) {
            auto element = text(state.label);
            element = applyStyle(std::move(element), option.look.style);
            if (option.look.color != ftxui::Color::Default) {
                element |= color(option.look.color);
            }
            if (state.active) {
                element = applyStyle(std::move(element), option.look.selected_style);
                if (option.look.selected_color != ftxui::Color::Default) {
                    element |= color(option.look.selected_color);
                }
            }
            if (state.focused) {
                element |= option.selectedDecorator;
            } else {
                element |= option.unselectedDecorator;
            }
            return element;
        };
        auto menuComponent = Menu(std::move(menuOption));
        auto borderStyle =
            option.look.border ? ftxui::BorderStyle::ROUNDED : ftxui::BorderStyle::EMPTY;
        auto menuRenderer = Renderer(menuComponent, [option, menu = menuComponent, borderStyle]() {
            auto content = menu->Render() | vscroll_indicator;
            if (option.look.border) {
                content = content | frame;
            }
            Element windowElement = option.look.border
                                        ? window(text(option.title), content, borderStyle)
                                        : vbox(text(option.title), content);

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
