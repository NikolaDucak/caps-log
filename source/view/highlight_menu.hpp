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

namespace clog::view {

using namespace ftxui;

class WindowedMenu : public ComponentBase {
    int m_selected;
public:
    WindowedMenu(std::string title, std::vector<std::string>* items, Ref<MenuOption> option) {
        auto menu  = Menu(items, &m_selected, option);
        auto menuRenderer = Renderer(menu, [=]() {
            return window(text(title), menu->Render() | frame) | size(WIDTH, LESS_THAN, 25);
        });
        Add(menuRenderer);
    }
    auto& selected() { return m_selected; }

    static auto make(std::string title, std::vector<std::string>* items, Ref<MenuOption> option){
        return std::make_shared<WindowedMenu>(title, items, option);
    }
};

}  // namespace clog::view
