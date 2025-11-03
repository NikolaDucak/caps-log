#include "annual_view_layout.hpp"

#include "calendar_component.hpp"
#include "fmt/core.h"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui_ext/extended_containers.hpp"
#include "utils/date.hpp"
#include "view/input_handler.hpp"
#include "view/view.hpp"
#include "view/windowed_menu.hpp"

#include <chrono>
#include <ftxui/screen/terminal.hpp>

#include <string>

namespace caps_log::view {
using std::function;

ftxui::Element identity(ftxui::Element element) { return element; }

namespace {
int daysDifference(std::chrono::year_month_day fromYmd, std::chrono::year_month_day toYmd) {
    std::chrono::sys_days fromSys{fromYmd};
    std::chrono::sys_days toSys{toYmd};
    return (toSys - fromSys).count();
}

} // namespace
using namespace ftxui;

AnnualViewLayout::AnnualViewLayout(InputHandlerBase *handler,
                                   function<ftxui::Dimensions()> screenSizeProvider,
                                   const std::chrono::year_month_day &today,
                                   ViewConfig::Logs config)
    : m_handler{handler}, m_screenSizeProvider{std::move(screenSizeProvider)}, m_today{today},
      m_calendarButtons{
          Calendar::make(m_screenSizeProvider, today, makeCalendarOptions(today, config.calendar))},
      m_tagsMenu{makeTagsMenu(config.tagsMenu)},
      m_sectionsMenu{makeSectionsMenu(config.sectionsMenu)},
      m_eventsList{makeEventsList(config.eventsList)},
      m_preview{makePreview(config.logEntryPreview)}, m_rootComponent{makeFullUIComponent()},
      m_recentEventsWindow{config.recentEventsWindow} {}

void AnnualViewLayout::showCalendarForYear(std::chrono::year year) {
    m_calendarButtons->displayYear(year);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::shared_ptr<ComponentBase> AnnualViewLayout::makeFullUIComponent() {
    const auto logContainer = ftxui_ext::CustomContainer(
        {
            m_sectionsMenu,
            m_tagsMenu,
            m_calendarButtons,
            m_eventsList,
            m_preview,
        },
        ftxui::Event::Tab, ftxui::Event::TabReverse);

    const auto dateStr = utils::date::formatToString(m_today, "%d. %m. %Y.");
    const auto wholeUiRenderer = Renderer(logContainer, [this, logContainer, dateStr,
                                                         firstRender = true]() mutable {
        // preview window can sometimes be wider than the menus & calendar, it's simpler to keep
        // them centered while the preview window changes and stretches this vbox container than
        // to keep the preview window size fixed
        const auto titleText = fmt::format(
            "Today is: {} {} | There are {} log entries for year {}.", dateStr, m_todaysEventString,
            m_datesWithLogs != nullptr ? m_datesWithLogs->size() : 0,
            static_cast<int>(m_calendarButtons->getFocusedDate().year()));

        static constexpr auto kMenuWidht = 25;
        static constexpr auto kMenuHeight = 20;
        // clang-format off
        auto mainSectionElements = Elements{
                hbox(
                    m_sectionsMenu->Render() | size(WIDTH, EQUAL, kMenuWidht) | size(HEIGHT, EQUAL, kMenuHeight), 
                    m_tagsMenu->Render() | size(WIDTH, EQUAL, kMenuWidht) | size(HEIGHT, EQUAL, kMenuHeight) 
                )
            };

        mainSectionElements.push_back(
            m_eventsList->Render()
                  | size(WIDTH, EQUAL, kMenuWidht * 2) 
                  | size(HEIGHT, LESS_THAN, kMenuHeight / 2) 
        );
        // clang-format on
        mainSectionElements = Elements{vbox(mainSectionElements)};

        mainSectionElements.push_back(m_calendarButtons->Render());
        const auto mainSection = hbox(mainSectionElements);

        static const auto kHelpString =
            std::string{"hjkl/arrow keys - navigation | d - delete log | s - see scratchpads | tab "
                        "- move focus between menus and calendar"};
        if (firstRender) {
            firstRender = false;
            m_handler->handleInputEvent(UIEvent{UiStarted{}});
        }
        static constexpr auto kPreviewFixedHeight = 14;
        // clang-format off
        return vbox(
            text(titleText) 
                | bold | underlined | center, 
            mainSection 
                | center,
            m_preview->Render()    
                | size(ftxui::HEIGHT, ftxui::EQUAL, kPreviewFixedHeight),
            text(kHelpString) 
                | dim | center
        ) | center | flex_grow;
        // clang-format on
    });

    const auto eventHandler = CatchEvent(wholeUiRenderer, [&](const ftxui::Event &event) {
        // controller does not care about mouse events
        if (not event.is_mouse()) {
            return m_handler->handleInputEvent(UIEvent{UnhandledRootEvent{event.input()}});
        }
        return false;
    });

    return eventHandler;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
CalendarOption AnnualViewLayout::makeCalendarOptions(const std::chrono::year_month_day &today,
                                                     ViewConfig::Logs::Calendar config) {
    CalendarOption option;

    auto toDecorator = [](ViewConfig::StyleMask style) {
        Decorator decorator = identity;
        if (style.bold) {
            decorator = decorator | bold;
        }
        if (style.underline) {
            decorator = decorator | underlined;
        }
        if (style.italic) {
            decorator = decorator | italic;
        }

        return decorator;
    };

    const auto weekdayStyle = toDecorator(config.weekdayStyle);
    const auto weekendStyle = toDecorator(config.weekendStyle);
    const auto todayStyle = toDecorator(config.todayStyle);
    const auto selectedDayStyle = toDecorator(config.selectedDayStyle);
    const auto eventDayStyle = toDecorator(config.eventDayStyle);

    option.transform = [this, today, &config, weekdayStyle, weekendStyle, todayStyle,
                        selectedDayStyle, eventDayStyle](const auto &date, const auto &state) {
        auto element = text(state.label);

        if (today == date) {
            element = element | color(config.todayColor) | todayStyle;
        } else if (m_highlightedDates &&
                   m_highlightedDates->contains(utils::date::monthDay(date))) {
            element = element | color(Color::Yellow);
        } else if (utils::date::isWeekend(date)) {
            element = element | color(config.weekendColor) | weekendStyle;
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
                    element = element | color(config.eventDayColor) | eventDayStyle;
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
    option.sundayStart = config.sundayStart;
    option.border = config.border;
    option.monthBorder = config.monthBorder;
    return option;
}

Component AnnualViewLayout::makeEventsList(const ViewConfig::Logs::EventsList &conf) {
    MenuOption menuOption;
    menuOption.entries = &m_recentAndUpcomingEventsList;

    menuOption.entries_option.transform = [this](const EntryState &state) {
        Element element = text(state.label);
        if (state.focused) {
            element = element | inverted;
        }
        if (state.active) {
            element = element | bold;
        }
        if (std::ranges::contains(m_recentAndUpcomingEventsGroupItemIndex, state.index)) {
            element = element | bold | underlined;
        }
        return element;
    };

    auto menuComponent = Menu(std::move(menuOption));
    auto borderStyle = conf.border ? BorderStyle::ROUNDED : BorderStyle::EMPTY;
    auto menuRenderer = Renderer(menuComponent, [menu = menuComponent, this, borderStyle]() {
        auto eventsCount =
            m_recentAndUpcomingEventsList.size() - m_recentAndUpcomingEventsGroupItemIndex.size();
        auto title = text("Recent & upcoming events (" + std::to_string(eventsCount) + ")");
        auto windowElement = window(title, menu->Render() | vscroll_indicator | frame, borderStyle);
        if (not menu->Focused()) {
            windowElement |= dim;
        }
        return windowElement;
    });
    return menuRenderer;
}

std::shared_ptr<WindowedMenu> AnnualViewLayout::makeTagsMenu(const ViewConfig::Logs::Menu &config) {
    WindowedMenuOption option{
        .title = "Tags",
        .entries = &m_tagMenuItems.getDisplayTexts(),
        .onChange = [this] { m_handler->handleInputEvent(UIEvent{FocusedTagChange{}}); },
        .border = config.border ? BorderStyle::ROUNDED : BorderStyle::EMPTY,
    };
    return WindowedMenu::make(option);
}

std::shared_ptr<WindowedMenu>
AnnualViewLayout::makeSectionsMenu(const ViewConfig::Logs::Menu &config) {
    WindowedMenuOption option = {
        .title = "Sections",
        .entries = &m_sectionMenuItems.getDisplayTexts(),
        .onChange = [this] { m_handler->handleInputEvent(UIEvent{FocusedSectionChange{}}); },
        .border = config.border ? BorderStyle::ROUNDED : BorderStyle::EMPTY,
    };
    return WindowedMenu::make(option);
}

std::shared_ptr<Preview>
AnnualViewLayout::makePreview(const ViewConfig::Logs::LogEntryPreview &config) {
    PreviewOptions options;
    options.border = config.border;
    options.markdownSyntaxHighlighting = config.markdownSyntaxHighlighting;
    return std::make_shared<Preview>(options);
}

void AnnualViewLayout::setPreviewString(const std::string &title, const std::string &string) {
    m_preview->setContent(title, string);
}

void AnnualViewLayout::setSelectedTag(std::string tag) {
    const auto tagInMenu =
        std::find(m_tagMenuItems.getKeys().begin(), m_tagMenuItems.getKeys().end(), tag);
    const auto newSelected = tagInMenu != m_tagMenuItems.getKeys().end()
                                 ? std::distance(m_tagMenuItems.getKeys().begin(), tagInMenu)
                                 : 0;
    m_tagsMenu->selected() = static_cast<int>(newSelected);
}

void AnnualViewLayout::setSelectedSection(std::string section) {
    const auto sectionInMenu = std::find(m_sectionMenuItems.getKeys().begin(),
                                         m_sectionMenuItems.getKeys().end(), section);
    const auto newSelected =
        sectionInMenu != m_sectionMenuItems.getKeys().end()
            ? std::distance(m_sectionMenuItems.getKeys().begin(), sectionInMenu)
            : 0;
    m_sectionsMenu->selected() = static_cast<int>(newSelected);
}

const std::string &AnnualViewLayout::getSelectedTag() const {
    return m_tagMenuItems.getKeys().at(m_tagsMenu->selected());
}

const std::string &AnnualViewLayout::getSelectedSection() const {
    return m_sectionMenuItems.getKeys().at(m_sectionsMenu->selected());
}

std::chrono::year_month_day AnnualViewLayout::getFocusedDate() const {
    return m_calendarButtons->getFocusedDate();
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void AnnualViewLayout::setEventDates(const CalendarEvents *events) {
    m_eventDates = events;
    const auto [eventItems, groupItemIndex, todaysEventStr] = [this] {
        std::vector<std::string> eventItems;
        std::vector<std::size_t> groupItemIndex;
        std::string todaysEventStr;
        for (const auto &[group, events] : *m_eventDates) {
            std::vector<std::string> groupItems;
            for (const auto &event : events) {
                const auto daysToEvent = daysDifference(
                    m_today, std::chrono::year_month_day(m_today.year(), event.date.month(),
                                                         event.date.day()));
                if (std::abs(daysToEvent) > m_recentEventsWindow) {
                    continue;
                }
                const auto daysStr = [daysToEvent]() -> std::string {
                    if (daysToEvent == 0) {
                        return "today";
                    }
                    if (daysToEvent < 0) {
                        return fmt::format("{} days ago", -daysToEvent);
                    }
                    return fmt::format("in {} days", daysToEvent);
                }();
                const auto dateStr = fmt::format("{:02}.{:02}.", (unsigned)event.date.month(),
                                                 (unsigned)event.date.day());
                groupItems.push_back(fmt::format(" - {} {} ({})", event.name, dateStr, daysStr));
                // check if the event is today
                if (event.date == utils::date::monthDay(m_today)) {
                    todaysEventStr = fmt::format("{} - {}", group, event.name);
                }
            }
            if (not groupItems.empty()) {
                eventItems.push_back(group);
                groupItemIndex.push_back(eventItems.size() - 1);
                eventItems.insert(eventItems.end(), groupItems.begin(), groupItems.end());
            }
        }
        return std::make_tuple(eventItems, groupItemIndex, todaysEventStr);
    }();
    m_recentAndUpcomingEventsList = eventItems;
    m_recentAndUpcomingEventsGroupItemIndex = groupItemIndex;
    m_todaysEventString = todaysEventStr;
}

void AnnualViewLayout::setHighlightedDates(const utils::date::Dates *map) {
    m_highlightedDates = map;
}

void AnnualViewLayout::setDatesWithLogs(const utils::date::Dates *map) { m_datesWithLogs = map; }

MenuItems &AnnualViewLayout::tagMenuItems() { return m_tagMenuItems; }

MenuItems &AnnualViewLayout::sectionMenuItems() { return m_sectionMenuItems; }

ftxui::Component AnnualViewLayout::getComponent() { return m_rootComponent; }

} // namespace caps_log::view
