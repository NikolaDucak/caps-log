#include "annual_view.hpp"

#include "calendar_component.hpp"
#include "fmt/core.h"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui_ext/extended_containers.hpp"
#include "view/input_handler.hpp"
#include "view/windowed_menu.hpp"

#include <ftxui/screen/terminal.hpp>

#include <string>

namespace caps_log::view {

using namespace ftxui;

AnnualView::AnnualView(const std::chrono::year_month_day &today, bool sundayStart)
    : m_screen{ScreenInteractive::Fullscreen()},
      m_calendarButtons{Calendar::make(m_screen, today, makeCalendarOptions(today, sundayStart))},
      m_tagsMenu{makeTagsMenu()}, m_sectionsMenu{makeSectionsMenu()},
      m_rootComponent{makeFullUIComponent()} {}

void AnnualView::run() { m_screen.Loop(m_rootComponent); }

void AnnualView::stop() { m_screen.ExitLoopClosure()(); }

void AnnualView::showCalendarForYear(std::chrono::year year) {
    m_calendarButtons->displayYear(year);
}

std::shared_ptr<Promptable> AnnualView::makeFullUIComponent() {
    const auto container = ftxui_ext::CustomContainer(
        {
            m_tagsMenu,
            m_sectionsMenu,
            m_calendarButtons,
            m_preview,
        },
        Event::Tab, Event::TabReverse);

    const auto wholeUiRenderer =
        Renderer(container, [this, container, firstRender = true]() mutable {
            // preview window can sometimes be wider than the menus & calendar, it's simpler to keep
            // them centered while the preview window changes and stretches this vbox container than
            // to keep the preview window size fixed
            const auto dateStr =
                utils::date::formatToString(utils::date::getToday(), "%d. %m. %Y.");
            const auto titleText =
                fmt::format("Today is: {} -- There are {} log entries for year {}.", dateStr,
                            m_availabeLogsMap != nullptr ? m_availabeLogsMap->daysSet() : 0,
                            static_cast<int>(m_calendarButtons->getFocusedDate().year()));
            const auto mainSection =
                hbox(m_tagsMenu->Render(), m_sectionsMenu->Render(), m_calendarButtons->Render());

            static const auto kHelpString =
                std::string{"hjkl/arrow keys - navigation | d - delete log "
                            "| tab - move focus between menus and calendar"};
            if (firstRender) {
                firstRender = false;
                m_screen.Post([this]() { m_handler->handleInputEvent(UIEvent{UiStarted{}}); });
            }
            return vbox(text(titleText) | center, mainSection | center, m_preview->Render(),
                        text(kHelpString) | dim | center) |
                   center;
        });

    const auto eventHandler = CatchEvent(wholeUiRenderer, [&](const Event &event) {
        // controller does not care about mouse events
        if (not event.is_mouse()) {
            return m_handler->handleInputEvent(UIEvent{UnhandledRootEvent{event.input()}});
        }
        return false;
    });

    return std::make_shared<Promptable>(eventHandler);
}

CalendarOption AnnualView::makeCalendarOptions(const std::chrono::year_month_day &today,
                                               bool sundayStart) {
    CalendarOption option;
    option.transform = [this, today](const auto &date, const auto &state) {
        auto element = text(state.label);

        if (today == date) {
            element = element | color(Color::Red);
        } else if (m_highlightedLogsMap && m_highlightedLogsMap->get(date)) {
            element = element | color(Color::Yellow);
        } else if (utils::date::isWeekend(date)) {
            element = element | color(Color::Blue);
        }

        if (state.focused) {
            element = element | inverted;
        }

        if (m_availabeLogsMap) {
            if (m_availabeLogsMap->get(date)) {
                element = element | underlined;
            } else {
                element = element | dim;
            }
        }

        return element | center;
    };
    option.focusChange = [this](const auto &date) {
        m_preview->resetScroll();
        m_handler->handleInputEvent(UIEvent{FocusedDateChange{date}});
    };
    option.enter = [this](const auto &date) {
        m_handler->handleInputEvent(UIEvent{OpenLogFile{date}});
    };
    option.sundayStart = sundayStart;
    return option;
}

std::shared_ptr<WindowedMenu> AnnualView::makeTagsMenu() {
    WindowedMenuOption option{
        .title = "Tags",
        .entries = &m_tagMenuItems,
        .onChange =
            [this] {
                m_handler->handleInputEvent(UIEvent{FocusedTagChange{m_tagsMenu->selected()}});
            },
    };
    return WindowedMenu::make(option);
}

std::shared_ptr<WindowedMenu> AnnualView::makeSectionsMenu() {
    WindowedMenuOption option = {
        .title = "Sections",
        .entries = &m_sectionMenuItems,
        .onChange =
            [this] {
                m_handler->handleInputEvent(
                    UIEvent{FocusedSectionChange{m_sectionsMenu->selected()}});
            },
    };
    return WindowedMenu::make(option);
}

void AnnualView::post(Task task) {
    m_screen.Post([task, this]() { std::get<Closure>(task)(); });
    m_screen.Post(Event::Custom);
}
void AnnualView::prompt(std::string message, std::function<void()> onYesCallback) {
    m_rootComponent->prompt(message, onYesCallback);
}
void AnnualView::promptOk(std::string message, std::function<void()> callback) {
    m_rootComponent->promptOk(message, callback);
}
void AnnualView::loadingScreen(const std::string &message) {
    m_rootComponent->loadingScreen(message);
}
void AnnualView::loadingScreenOff() { m_rootComponent->resetToMain(); }

} // namespace caps_log::view
