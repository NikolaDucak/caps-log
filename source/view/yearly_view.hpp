#pragma once

#include "../model/Date.hpp"
#include "../model/LogModel.hpp"
#include "calendar_component.hpp"
#include "highlight_menu.hpp"
#include "input_handler.hpp"

#include <array>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace clog::view {

using namespace ftxui;

class YearlyView {
    model::YearLogEntryData m_data;
    ScreenInteractive m_screen;
    CalendarComponent m_calendarButtons;
    std::shared_ptr<CalendarHighlighMenuComponent> m_tasksMenu, m_sectionsMenu;

    std::optional<model::Date> m_editorRequestForDate = {};
    ControlerBase* m_editorControler                  = nullptr;
    Component m_rootComponent;

public:

    YearlyView(const model::Date& today,model::YearLogEntryData& info);

    void run();
    void setContoler(ControlerBase* handler);
    void setLogAvailabilityForDate(const model::Date&, bool available);

    void setDisplayData(const model::YearLogEntryData& info);
    model::YearLogEntryData& getDisplayedData() { return m_data; }
    void updateComponents() {
        m_tasksMenu->dataChanged();
        m_sectionsMenu->dataChanged();
        m_calendarButtons.update();
        m_rootComponent = createUI();
    }

private:
    Component createUI();
};

}  // namespace clog::view
