#include "yearly_view.hpp"

#include "calendar_component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui_ext/extended_containers.hpp"

#include <ftxui/screen/terminal.hpp>
#include <sstream>

namespace clog::view {
YearlyView::YearlyView(const model::Date& today) :
    m_screen { ScreenInteractive::Fullscreen() },
    m_calendarButtons { Calendar::make(today, makeCalendarOptions(today)) },
    m_tagsMenu { WindowedMenu::make("Tags", &m_tags,
                                    MenuOption { .on_change =
                                                     [this] {
                                                         m_handler->handleInputEvent(
                                                             { UIEvent::FOCUSED_TAG_CHANGE,
                                                               m_tagsMenu->selected() });
                                                     } }) },
    m_sectionsMenu { WindowedMenu::make(
        "Sections", &m_sections,
        MenuOption { .on_change =
                         [this] {
                             m_handler->handleInputEvent(
                                 { UIEvent::FOCUSED_SECTION_CHANGE,
                                   m_sectionsMenu->selected() });
                         } }) },
    m_rootComponent { makeFullUIComponent() } {}

void YearlyView::run() { m_screen.Loop(m_rootComponent); }

void YearlyView::stop() { m_screen.ExitLoopClosure()(); }

std::shared_ptr<Promptable> YearlyView::makeFullUIComponent() {
    auto container = ftxui_ext::CustomContainer(
        {
            m_sectionsMenu,
            m_tagsMenu,
            m_calendarButtons,
        },
        Event::Tab, Event::TabReverse);

    auto whole_ui_renderer = Renderer(container, [this, container] {
        std::stringstream date;
        date << "Today: " << model::Date::getToday().formatToString("%d. %m. %Y.");
        return vbox(text(date.str()), container->Render(), m_logFileContentsPreview) | center;
    });

    auto event_handler = CatchEvent(whole_ui_renderer, [&](Event e) {
        if (e.is_character()) {
            // TODO: why is e.input() a string? Will i miss some data if i just take the first char?
            return m_handler->handleInputEvent({ UIEvent::ROOT_EVENT, e.input().front() });
        }
        return false;
    });

    return std::make_shared<Promptable>(event_handler);
}

CalendarOption YearlyView::makeCalendarOptions(const Date& today) {
    CalendarOption option;
    option.transform = [this, today](const auto& date, const auto& state) {
        auto element = text(state.label);
        if (state.focused)
            element = element | inverted;
        if (today == date)
            element = element | color(Color::Red);
        if (m_highlightedLogsMap && m_highlightedLogsMap->get(date))
            element = element | color(Color::Yellow);
        if (m_availabeLogsMap && !m_availabeLogsMap->get(date))
            element = element | dim;
        return element | center;
    };
    option.focusChange = [this](const auto& date) {
        // TODO: Empty strings in consturctors dont look nice. Plus, ignoring
        // the provided new date only for the controller to ask for new
        // date is even uglier
        m_handler->handleInputEvent({ UIEvent::FOCUSED_DATE_CHANGE});
    };
    option.enter = [this](const auto& date) {
        m_handler->handleInputEvent({ UIEvent::CALENDAR_BUTTON_CLICK });
    };
    return option;
}

}  // namespace clog::view
