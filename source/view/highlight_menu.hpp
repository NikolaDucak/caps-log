#pragma once

#include "../model/LogModel.hpp"  
#include "calendar_component.hpp"

#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * A UI component that is responsible of updating a colection of `day_button`s
 * `m_highlight` members based on some data that indicates wheather a button for
 * a certain date should be highlighted when a certain member of the menu is
 * clicked on.
 */
namespace clog::view {

using namespace ftxui;

class CalendarHighlighMenuComponent : public ComponentBase {
    /** 
     * A Container holding the display strings of items and strings that
     * are actual keys in the map of availability.
     */
    class MenuItems {
        std::vector<std::string> m_items, m_itemDisplayStrings;
        std::vector<std::string> getDisplayStrings() { return m_itemDisplayStrings; }
        void pushNewItem(std::string item, std::string display);
    };
    model::StringYearMap& m_info;
    std::pair<std::vector<std::string>, std::vector<std::string>> m_items;
    int m_selected = 0;
    model::YearMap<bool>*& m_selectedOutput;
    Component m_menu;
    const std::string m_title;

public:
    CalendarHighlighMenuComponent(model::StringYearMap& map, model::YearMap<bool>*& selected);

    /**
     * In case the m_info has changed we need to recount all the menu items and 
     * rebuild the FTXUI menu.
     */
    void dataChanged();

private:

    Component createFTXUIMenu();

};

}  // namespace clog::view
