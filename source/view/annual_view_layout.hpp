#pragma once

#include "annual_view_layout_base.hpp"
#include "calendar_component.hpp"
#include "input_handler.hpp"
#include "preview.hpp"
#include "utils/date.hpp"
#include "view/view.hpp"
#include "windowed_menu.hpp"

#include <chrono>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>
#include <ftxui/dom/elements.hpp>

namespace caps_log::view {

class AnnualViewLayout : public AnnualViewLayoutBase {
    InputHandlerBase *m_handler{nullptr};
    std::function<ftxui::Dimensions()> m_screenSizeProvider;
    std::chrono::year_month_day m_today;

    // UI components visible to the user
    std::shared_ptr<Calendar> m_calendarButtons;
    std::shared_ptr<WindowedMenu> m_tagsMenu;
    std::shared_ptr<WindowedMenu> m_sectionsMenu;
    ftxui::Component m_eventsList;
    std::shared_ptr<Preview> m_preview = std::make_unique<Preview>();
    // Maps that help m_calendarButtons highlight certain logs.
    const utils::date::Dates *m_highlightedDates = nullptr;
    const utils::date::Dates *m_datesWithLogs = nullptr;
    const CalendarEvents *m_eventDates = nullptr;

    // Menu items for m_tagsMenu & m_sectionsMenu
    MenuItems m_tagMenuItems, m_sectionMenuItems;

    unsigned m_recentEventsWindow;

    // Events
    std::vector<std::string> m_recentAndUpcomingEventsList;
    std::vector<std::size_t> m_recentAndUpcomingEventsGroupItemIndex;
    std::string m_todaysEventString;

    std::shared_ptr<ftxui::ComponentBase> m_rootComponent;

  public:
    AnnualViewLayout(InputHandlerBase *handler,
                     std::function<ftxui::Dimensions()> screenSizeProvider,
                     const std::chrono::year_month_day &today, ViewConfig::Logs config);

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
    std::shared_ptr<WindowedMenu> makeTagsMenu(const ViewConfig::Logs::Menu &config);
    std::shared_ptr<WindowedMenu> makeSectionsMenu(const ViewConfig::Logs::Menu &config);
    static std::shared_ptr<Preview> makePreview(const ViewConfig::Logs::LogEntryPreview &config);
    ftxui::Component makeEventsList(const ViewConfig::Logs::EventsList &config);
    CalendarOption makeCalendarOptions(const std::chrono::year_month_day &today,
                                       ViewConfig::Logs::Calendar config);
};

} // namespace caps_log::view
