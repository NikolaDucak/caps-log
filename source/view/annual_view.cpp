#include "annual_view.hpp"

#include "calendar_component.hpp"
#include "fmt/core.h"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui_ext/extended_containers.hpp"
#include "utils/date.hpp"
#include "view/input_handler.hpp"
#include "view/windowed_menu.hpp"

#include <chrono>
#include <ftxui/screen/terminal.hpp>

#include <ranges>
#include <string>

namespace caps_log::view {

namespace {
int daysDifference(std::chrono::year_month_day from_ymd, std::chrono::year_month_day to_ymd) {
    std::chrono::sys_days fromSys{from_ymd};
    std::chrono::sys_days toSys{to_ymd};
    return (toSys - fromSys).count();
}
} // namespace
using namespace ftxui;

AnnualView::AnnualView(const std::chrono::year_month_day &today, bool sundayStart,
                       unsigned recentEventsWindow)
    : m_screen{ScreenInteractive::Fullscreen()},
      m_calendarButtons{Calendar::make(ScreenSizeProvider::makeDefault(), today,
                                       makeCalendarOptions(today, sundayStart))},
      m_tagsMenu{makeTagsMenu()}, m_sectionsMenu{makeSectionsMenu()},
      m_rootComponent{makeFullUIComponent()}, m_recentEventsWindow{recentEventsWindow} {}

void AnnualView::run() {
    // Caps-log expects the terminal to be at least 80x24
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    Terminal::SetFallbackSize(Dimensions{/*dimx=*/80, /*dimy=*/24});
    m_screen.Loop(m_rootComponent);
}

void AnnualView::stop() { m_screen.ExitLoopClosure()(); }

void AnnualView::showCalendarForYear(std::chrono::year year) {
    m_calendarButtons->displayYear(year);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::shared_ptr<Promptable> AnnualView::makeFullUIComponent() {
    const auto container = ftxui_ext::CustomContainer(
        {
            m_sectionsMenu,
            m_tagsMenu,
            m_calendarButtons,
            m_preview,
        },
        ftxui::Event::Tab, ftxui::Event::TabReverse);

    const auto wholeUiRenderer = Renderer(container, [this, container,
                                                      firstRender = true]() mutable {
        // preview window can sometimes be wider than the menus & calendar, it's simpler to keep
        // them centered while the preview window changes and stretches this vbox container than
        // to keep the preview window size fixed
        const auto dateStr = utils::date::formatToString(utils::date::getToday(), "%d. %m. %Y.");
        const auto todaysEvent = [this] {
            if (m_eventDates) {
                for (const auto &[group, events] : *m_eventDates) {
                    const auto it = std::ranges::find_if(events, [](const auto &event) {
                        return event.date == utils::date::monthDay(utils::date::getToday());
                    });
                    if (it != events.end()) {
                        return fmt::format("{} - {}", group, it->name);
                    }
                }
            }
            return std::string{};
        }();
        const auto titleText =
            fmt::format("Today is: {} {} -- There are {} log entries for year {}.", dateStr,
                        todaysEvent.empty() ? "" : fmt::format("({})", todaysEvent),
                        m_datesWithLogs != nullptr ? m_datesWithLogs->size() : 0,
                        static_cast<int>(m_calendarButtons->getFocusedDate().year()));

        Elements eventItems;
        for (const auto &[group, events] : *m_eventDates) {
            Elements groupItems;
            for (const auto &event : events) {
                const auto today = utils::date::getToday();
                const auto daysToEvent =
                    daysDifference(today, std::chrono::year_month_day(
                                              today.year(), event.date.month(), event.date.day()));
                if (std::abs(daysToEvent) > m_recentEventsWindow) {
                    continue;
                }
                const auto daysStr = daysToEvent > 0 ? fmt::format("in {} days", daysToEvent)
                                                     : fmt::format("{} days ago", -daysToEvent);
                const auto dateStr = fmt::format("{:02}.{:02}.", (unsigned)event.date.month(),
                                                 (unsigned)event.date.day());
                groupItems.push_back(
                    text(fmt::format("  - {} {} ({})", event.name, dateStr, daysStr)));
            }
            if (not groupItems.empty()) {
                eventItems.push_back(text(group) | bold);
                eventItems.push_back(vbox(std::move(groupItems)));
            }
        }

        const auto mainSection =
            (eventItems.empty()) ? hbox(vbox(hbox(m_sectionsMenu->Render(), m_tagsMenu->Render())),
                                        m_calendarButtons->Render())
                                 : hbox(vbox(hbox(m_sectionsMenu->Render(), m_tagsMenu->Render()),
                                             vbox(eventItems) | border),
                                        m_calendarButtons->Render());

        static const auto kHelpString =
            std::string{"hjkl/arrow keys - navigation | d - delete log "
                        "| tab - move focus between menus and calendar"};
        if (firstRender) {
            firstRender = false;
            m_screen.Post([this]() { m_handler->handleInputEvent(UIEvent{UiStarted{}}); });
        }
        return vbox(text(titleText) | bold | underlined | center, mainSection | center,
                    m_preview->Render(), text(kHelpString) | dim | center) |
               center;
    });

    const auto eventHandler = CatchEvent(wholeUiRenderer, [&](const ftxui::Event &event) {
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
        } else if (m_highlightedDates &&
                   m_highlightedDates->contains(utils::date::monthDay(date))) {
            element = element | color(Color::Yellow);
        } else if (utils::date::isWeekend(date)) {
            element = element | color(Color::Blue);
        }

        if (state.focused) {
            element = element | inverted;
        }

        if (m_datesWithLogs) {
            if (m_datesWithLogs->contains(utils::date::monthDay(date))) {
                element = element | underlined;
            } else {
                element = element | dim;
            }
        }

        if (m_eventDates) {
            for (const auto &[_, events] : *m_eventDates) {
                const auto isEvent = std::ranges::any_of(events, [date](const auto &event) {
                    return event.date == utils::date::monthDay(date);
                });
                if (isEvent) {
                    element = element | bgcolor(Color::Green);
                    break;
                }
            }
        }

        return element | center;
    };
    option.focusChange = [this](const auto &date) {
        m_preview->resetScroll();
        m_handler->handleInputEvent(UIEvent{FocusedDateChange{}});
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
        .entries = &m_tagMenuItems.getDisplayTexts(),
        .onChange = [this] { m_handler->handleInputEvent(UIEvent{FocusedTagChange{}}); },
    };
    return WindowedMenu::make(option);
}

std::shared_ptr<WindowedMenu> AnnualView::makeSectionsMenu() {
    WindowedMenuOption option = {
        .title = "Sections",
        .entries = &m_sectionMenuItems.getDisplayTexts(),
        .onChange = [this] { m_handler->handleInputEvent(UIEvent{FocusedSectionChange{}}); },
    };
    return WindowedMenu::make(option);
}

void AnnualView::post(Task task) {
    m_screen.Post([task, this]() { std::get<Closure>(task)(); });
    m_screen.Post(ftxui::Event::Custom);
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
