#include "yearly_view.hpp"

#include "ftxui/component/screen_interactive.hpp"
#include "ftxui_ext/extended_containers.hpp"

#include <ftxui/screen/terminal.hpp>
#include <sstream>

namespace clog::view {

YearlyView::YearlyView(const model::Date& today, model::YearLogEntryData& info) :
    m_data { info }, m_screen { ScreenInteractive::Fullscreen() },
    m_calendarButtons { today, &m_data.logAvailabilityMap,
                        [this](const auto& date) {
                            m_editorRequestForDate = date;
                            m_screen.ExitLoopClosure()();
                        } },
    m_tasksMenu { std::make_shared<CalendarHighlighMenuComponent>(
        m_data.taskMap, m_calendarButtons.m_selectedHighlightMap) },
    m_sectionsMenu { std::make_shared<CalendarHighlighMenuComponent>(
        m_data.sectionMap, m_calendarButtons.m_selectedHighlightMap) },
    m_rootComponent { createUI() } {}

void YearlyView::run() {
    while (true) {
        m_screen.Loop(m_rootComponent);
        // NOTE: stoping and restarting the loop in button callback
        // will cause a segfault
        if (m_editorRequestForDate && m_editorControler) {
            m_editorControler->openLogEntryInEditorForDade(*m_editorRequestForDate);
            m_editorRequestForDate = {};
        } else {
            break;
        }
    }
}

void YearlyView::setDisplayData(const clog::model::YearLogEntryData& info) { m_data = info; }

void YearlyView::setContoler(ControlerBase* handler) { m_editorControler = handler; }

Component YearlyView::createUI() {
    auto container = ftxui_ext::CustomContainer(
        {
            m_sectionsMenu,
            m_tasksMenu,
            m_calendarButtons.getFTXUIComponent(),
        },
        Event::Tab, Event::TabReverse);

    auto whole_ui_renderer = Renderer(container, [=] {
        std::stringstream date;
        date << "Today: " << model::Date::getToday().formatToString("%d. %m. %Y.");
        return vbox(text(date.str()) | center, container->Render()) | center;
    });

    auto quit_on_esc_event = CatchEvent(whole_ui_renderer, [&](auto e) {
        if (e == Event::Escape) {
            m_screen.ExitLoopClosure()();
            return true;
        }
        if (e ==Event::Character('d') ) {
            // prompt "u sure?"
        }
        return false;
    });

    return quit_on_esc_event;
}

}  // namespace clog::view
