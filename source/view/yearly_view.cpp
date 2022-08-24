#include "yearly_view.hpp"

#include "calendar_component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/flexbox_config.hpp"
#include "ftxui_ext/extended_containers.hpp"

#include <ftxui/screen/terminal.hpp>
#include <sstream>

#include <string>

namespace clog::view {

YearView::YearView(const date::Date &today, bool sundayStart)
    : m_screen{ScreenInteractive::Fullscreen()},
      m_calendarButtons{Calendar::make(today, makeCalendarOptions(today, sundayStart))},
      m_tagsMenu{makeTagsMenu()}, m_sectionsMenu{makeSectionsMenu()}, m_rootComponent{
                                                                          makeFullUIComponent()} {}

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
        std::stringstream date;
        date << "Today: " << date::Date::getToday().formatToString("%d. %m. %Y.");
        // preview window can sometimes be wider than the menus & calendar, it's simpler to keep
        // them centered while the preview window changes and stretches this vbox container than to
        // keep the preview window size fixed
        auto main_section =
            hbox(m_tagsMenu->Render(), m_sectionsMenu->Render(), m_calendarButtons->Render());
        return vbox(text(date.str()) | center, main_section | center, m_preview->Render()) | center;
    });

    auto event_handler = CatchEvent(whole_ui_renderer, [&](const Event &event) {
        // TODO: gotta prevent tab reverse error
        if (not event.is_mouse() && event != Event::TabReverse) {
            return m_handler->handleInputEvent({UIEvent::ROOT_EVENT, event.input()});
        };
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

        if (m_availabeLogsMap && !m_availabeLogsMap->get(date))
            element = element | dim;

        return element | center;
    };
    // TODO: Ignoring the provided new date only for the controller to ask
    // for new date is ugly
    option.focusChange = [this](const auto & /* date */) {
        m_handler->handleInputEvent({UIEvent::FOCUSED_DATE_CHANGE});
    };
    option.enter = [this](const auto & /* date */) {
        m_handler->handleInputEvent({UIEvent::CALENDAR_BUTTON_CLICK});
    };
    option.sundayStart = sundayStart;
    return std::move(option);
}

std::shared_ptr<WindowedMenu> YearView::makeTagsMenu() {
    MenuOption option{.on_change = [this] {
        m_handler->handleInputEvent(
            {UIEvent::FOCUSED_TAG_CHANGE, std::to_string(m_tagsMenu->selected())});
    }};
    return WindowedMenu::make("Tags", &m_tagMenuItems, option);
}

std::shared_ptr<WindowedMenu> YearView::makeSectionsMenu() {
    MenuOption option = {.on_change = [this] {
        m_handler->handleInputEvent(
            {UIEvent::FOCUSED_SECTION_CHANGE, std::to_string(m_sectionsMenu->selected())});
    }};
    return WindowedMenu::make("Sections", &m_sectionMenuItems, option);
}
} // namespace clog::view
