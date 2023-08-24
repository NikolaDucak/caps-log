#include "yearly_view.hpp"

#include "calendar_component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/flexbox_config.hpp"
#include "ftxui_ext/extended_containers.hpp"

#include "fmt/format.h"
#include <ftxui/screen/terminal.hpp>
#include <sstream>

#include <string>

namespace clog::view {

YearView::YearView(const date::Date &today, bool sundayStart)
    : m_screen{ScreenInteractive::Fullscreen()},
      m_calendarButtons{Calendar::make(today, makeCalendarOptions(today, sundayStart))},
      m_tagsMenu{makeTagsMenu()}, m_sectionsMenu{makeSectionsMenu()},
      m_rootComponent{makeFullUIComponent()} {}

void YearView::run() { m_screen.Loop(m_rootComponent); }

void YearView::stop() { m_screen.ExitLoopClosure()(); }

void YearView::showCalendarForYear(unsigned year) { m_calendarButtons->displayYear(year); }

std::shared_ptr<Promptable> YearView::makeFullUIComponent() {
    auto container = ftxui_ext::CustomContainer(
        {
            m_tagsMenu,
            m_sectionsMenu,
            m_calendarButtons,
            m_preview,
        },
        Event::Tab, Event::TabReverse);

    auto whole_ui_renderer = Renderer(container, [this, container] {
        // preview window can sometimes be wider than the menus & calendar, it's simpler to keep
        // them centered while the preview window changes and stretches this vbox container than to
        // keep the preview window size fixed
        const auto dateStr = date::Date::getToday().formatToString("%d. %m. %Y.");
        const auto titleText =
            fmt::format("Today is: {} -- There are {} log entries for year {}.", dateStr,
                        m_availabeLogsMap ? m_availabeLogsMap->daysSet() : 0,
                        m_calendarButtons->getFocusedDate().year);
        const auto main_section =
            hbox(m_tagsMenu->Render(), m_sectionsMenu->Render(), m_calendarButtons->Render());

        static const auto helpString = std::string{"hjkl/arrow keys - navigation | d - delete log "
                                                   "| tab - move focus between menus and calendar"};
        return vbox(text(titleText) | center, main_section | center, m_preview->Render(),
                    text(helpString) | dim | center) |
               center;
    });

    auto event_handler = CatchEvent(whole_ui_renderer, [&](const Event &event) {
        // controller does not care about mouse events
        if (not event.is_mouse()) {
            return m_handler->handleInputEvent({UIEvent::ROOT_EVENT, event.input()});
        }
        return false;
    });

    return std::make_shared<Promptable>(event_handler);
}

CalendarOption YearView::makeCalendarOptions(const Date &today, bool sundayStart) {
    CalendarOption option;
    option.transform = [this, today](const auto &date, const auto &state) {
        auto element = text(state.label);

        if (today == date)
            element = element | color(Color::Red);
        else if (m_highlightedLogsMap && m_highlightedLogsMap->get(date))
            element = element | color(Color::Yellow);
        else if (date.isWeekend())
            element = element | color(Color::Blue);

        if (state.focused)
            element = element | inverted;

        if (m_availabeLogsMap) {
            if (m_availabeLogsMap->get(date))
                element = element | underlined;
            else
                element = element | dim;
        }

        return element | center;
    };
    // TODO: Ignoring the provided new date only for the controller to ask
    // for new date is ugly
    option.focusChange = [this](const auto & /* date */) {
        m_preview->resetScroll();
        m_handler->handleInputEvent({UIEvent::FOCUSED_DATE_CHANGE});
    };
    option.enter = [this](const auto & /* date */) {
        m_handler->handleInputEvent({UIEvent::CALENDAR_BUTTON_CLICK});
    };
    option.sundayStart = sundayStart;
    return std::move(option);
}

std::shared_ptr<WindowedMenu> YearView::makeTagsMenu() {
    MenuOption option{
        .entries = &m_tagMenuItems,
        .on_change =
            [this] {
                m_handler->handleInputEvent(
                    {UIEvent::FOCUSED_TAG_CHANGE, std::to_string(m_tagsMenu->selected())});
            },
    };
    return WindowedMenu::make("Tags", option);
}

std::shared_ptr<WindowedMenu> YearView::makeSectionsMenu() {
    MenuOption option = {
        .entries = &m_sectionMenuItems,
        .on_change =
            [this] {
                m_handler->handleInputEvent(
                    {UIEvent::FOCUSED_SECTION_CHANGE, std::to_string(m_sectionsMenu->selected())});
            },
    };
    return WindowedMenu::make("Sections", option);
}
} // namespace clog::view
