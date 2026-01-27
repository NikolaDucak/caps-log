#pragma once

#include "annual_view_layout_base.hpp"
#include "calendar_component.hpp"
#include "input_handler.hpp"
#include "preview.hpp"
#include "utils/date.hpp"
#include "windowed_menu.hpp"

#include <chrono>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>
#include <ftxui/dom/elements.hpp>

namespace caps_log::view {

// Configuration structs for various UI components, currently only holding border settings.
struct MenuConfig {
    ftxui::BorderStyle border;
};

struct PreviewConfig {
    ftxui::BorderStyle border;
};

struct EventsListConfig {
    ftxui::BorderStyle border;
};

struct TextPreviewConfig {
    ftxui::BorderStyle border;
};

struct FtxuiTheme {
    ftxui::Decorator emptyDateDecorator;
    ftxui::Decorator logDateDecorator;
    ftxui::Decorator weekendDateDecorator;
    ftxui::Decorator eventDateDecorator;
    ftxui::Decorator highlightedDateDecorator;
    ftxui::Decorator todaysDateDecorator;
    ftxui::BorderStyle calendarBorder;
    ftxui::BorderStyle calendarMonthBorder;
    MenuConfig tagsMenuConfig;
    MenuConfig sectionsMenuConfig;
    EventsListConfig eventsListConfig;
    TextPreviewConfig logEntryPreviewConfig;
};
struct AnnualViewConfig {
    FtxuiTheme theme;
    bool sundayStart = false;
    unsigned recentEventsWindow = 0;
};

class AnnualViewLayout : public AnnualViewLayoutBase {
    InputHandlerBase *m_handler{nullptr};
    std::function<ftxui::Dimensions()> m_screenSizeProvider;
    std::chrono::year_month_day m_today;
    AnnualViewConfig m_config;

    // UI components visible to the user
    std::shared_ptr<Calendar> m_calendarButtons;
    std::shared_ptr<WindowedMenu> m_tagsMenu;
    std::shared_ptr<WindowedMenu> m_sectionsMenu;
    ftxui::Component m_eventsList;
    std::shared_ptr<Preview> m_preview = std::make_unique<Preview>(
        PreviewOption{.border = m_config.theme.logEntryPreviewConfig.border});
    // Maps that help m_calendarButtons highlight certain logs.
    const utils::date::Dates *m_highlightedDates = nullptr;
    const utils::date::Dates *m_datesWithLogs = nullptr;
    const CalendarEvents *m_eventDates = nullptr;

    // Menu items for m_tagsMenu & m_sectionsMenu
    MenuItems m_tagMenuItems, m_sectionMenuItems;

    // Events
    std::vector<std::string> m_recentAndUpcomingEventsList;
    std::vector<std::size_t> m_recentAndUpcomingEventsGroupItemIndex;
    std::string m_todaysEventString;

    std::shared_ptr<ftxui::ComponentBase> m_rootComponent;

  public:
    AnnualViewLayout(InputHandlerBase *handler,
                     std::function<ftxui::Dimensions()> screenSizeProvider,
                     const std::chrono::year_month_day &today, AnnualViewConfig config);

    void showCalendarForYear(std::chrono::year year) override;

    void setDatesWithLogs(const utils::date::Dates *map) override;
    void setHighlightedDates(const utils::date::Dates *map) override;
    void setEventDates(const CalendarEvents *events) override;

    void setPreviewString(const std::string &title, const std::string &string) override;

    [[nodiscard]] std::chrono::year_month_day getFocusedDate() const override;

    [[nodiscard]] MenuItems &tagMenuItems() override;
    [[nodiscard]] MenuItems &sectionMenuItems() override;

    void setSelectedTag(std::string tag) override;

    void setSelectedSection(std::string section) override;

    [[nodiscard]] const std::string &getSelectedTag() const override;

    [[nodiscard]] const std::string &getSelectedSection() const override;

    ftxui::Component getComponent() override;

  private:
    std::shared_ptr<ftxui::ComponentBase> makeFullUIComponent();
    std::shared_ptr<WindowedMenu> makeTagsMenu();
    std::shared_ptr<WindowedMenu> makeSectionsMenu();
    ftxui::Component makeEventsList();
    CalendarOption makeCalendarOptions(const std::chrono::year_month_day &today);
};

} // namespace caps_log::view
