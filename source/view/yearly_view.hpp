#pragma once

#include "../model/Date.hpp"
#include "calendar_component.hpp"
#include "highlight_menu.hpp"
#include "input_handler.hpp"
#include "promptable.hpp"

#include <array>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>
namespace clog::view {

using namespace ftxui;


class YearlyView {
    InputHandlerBase* m_handler;
    ScreenInteractive m_screen;
    std::shared_ptr<Calendar> m_calendarButtons;
    std::shared_ptr<WindowedMenu> m_tagsMenu, m_sectionsMenu;
    std::shared_ptr<Promptable> m_rootComponent;
    Element m_logFileContentsPreview;
    const model::YearMap<bool>* m_highlightedLogsMap = nullptr;
    const model::YearMap<bool>* m_availabeLogsMap    = nullptr;

    std::vector<std::string> m_tags {}, m_sections {};

public:
    YearlyView(const model::Date& today);

    void run();
    void stop();

    void showCalendarForYear(unsigned year) {
        m_calendarButtons->displayYear(year);
    }

    void prompt(std::string message, std::function<void()> onYesCallback) {
        m_rootComponent->prompt(message, onYesCallback);
    }

    int& selectedTag() { return m_tagsMenu->selected(); }
    int& selectedSection() { return m_sectionsMenu->selected(); }
    
    void setInputHandler(InputHandlerBase* handler) { m_handler = handler; }

    void setAvailableLogsMap(const model::YearMap<bool>* map) { m_availabeLogsMap = map; }
    void setHighlightedLogsMap(const model::YearMap<bool>* map) { m_highlightedLogsMap = map; }

    void setTagMenuItems(std::vector<std::string> items) {
        m_tags       = std::move(items);
    }

    void setSectionMenuItems(std::vector<std::string> items) {
        m_sections       = std::move(items);
    }

    void setPreviewString(const std::string& string) {
        Elements lines;
        std::istringstream input { string };
        for (std::string line; std::getline(input, line);) {
            lines.push_back(text(line));
        }
        m_logFileContentsPreview = vbox(lines) | border | size(HEIGHT, EQUAL, 10);
    }

    // temporay restore terminal to its roriginal state
    // to executa a function. used to start the editor
    void withRestoredIO(std::function<void()> func) {
        m_screen.WithRestoredIO(func)();
    }

    model::Date getFocusedDate() const { return m_calendarButtons->getFocusedDate(); }

private:
    std::shared_ptr<Promptable> makeFullUIComponent();
    CalendarOption makeCalendarOptions(const Date& today);
};

}  // namespace clog::view
