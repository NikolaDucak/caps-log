#pragma once

#include "annual_view_base.hpp"
#include "calendar_component.hpp"
#include "input_handler.hpp"
#include "preview.hpp"
#include "promptable.hpp"
#include "utils/date.hpp"
#include "windowed_menu.hpp"

#include <chrono>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>
#include <ftxui/dom/elements.hpp>

namespace caps_log::view {

class AnnualView : public AnnualViewBase {
    InputHandlerBase *m_handler{nullptr};
    ftxui::ScreenInteractive m_screen;

    // UI components visible to the user
    std::shared_ptr<Calendar> m_calendarButtons;
    std::shared_ptr<WindowedMenu> m_tagsMenu, m_sectionsMenu;
    std::shared_ptr<Preview> m_preview = std::make_unique<Preview>();
    std::shared_ptr<Promptable> m_rootComponent;

    // Maps that help m_calendarButtons highlight certain logs.
    const utils::date::Dates *m_highlightedDates = nullptr;
    const utils::date::Dates *m_datesWithLogs = nullptr;
    const CalendarEvents *m_eventDates = nullptr;

    // Menu items for m_tagsMenu & m_sectionsMenu
    MenuItems m_tagMenuItems, m_sectionMenuItems;

    unsigned m_recentEventsWindow;

  public:
    AnnualView(const std::chrono::year_month_day &today, bool sundayStart,
               unsigned recentEventsWindow);

    void run() override;
    void stop() override;

    void post(ftxui::Task task) override;

    void prompt(std::string message, std::function<void()> onYesCallback) override;
    void promptOk(std::string message, std::function<void()> callback) override;
    void loadingScreen(const std::string &message) override;
    void loadingScreenOff() override;

    void showCalendarForYear(std::chrono::year year) override;

    void setInputHandler(InputHandlerBase *handler) override { m_handler = handler; }

    void setDatesWithLogs(const utils::date::Dates *map) override { m_datesWithLogs = map; }
    void setHighlightedDates(const utils::date::Dates *map) override { m_highlightedDates = map; }
    void setEventDates(const CalendarEvents *events) override { m_eventDates = events; }

    void setPreviewString(const std::string &title, const std::string &string) override {
        m_preview->setContent(title, string);
    }

    void withRestoredIO(std::function<void()> func) override { m_screen.WithRestoredIO(func)(); }

    std::chrono::year_month_day getFocusedDate() const override {
        return m_calendarButtons->getFocusedDate();
    }

    [[nodiscard]] MenuItems &tagMenuItems() override { return m_tagMenuItems; }
    [[nodiscard]] MenuItems &sectionMenuItems() override { return m_sectionMenuItems; }

    void setSelectedTag(std::string tag) override {
        const auto tagInMenu =
            std::find(m_tagMenuItems.getKeys().begin(), m_tagMenuItems.getKeys().end(), tag);
        const auto newSelected = tagInMenu != m_tagMenuItems.getKeys().end()
                                     ? std::distance(m_tagMenuItems.getKeys().begin(), tagInMenu)
                                     : 0;
        m_tagsMenu->selected() = static_cast<int>(newSelected);
    }

    void setSelectedSection(std::string section) override {
        const auto sectionInMenu = std::find(m_sectionMenuItems.getKeys().begin(),
                                             m_sectionMenuItems.getKeys().end(), section);
        const auto newSelected =
            sectionInMenu != m_sectionMenuItems.getKeys().end()
                ? std::distance(m_sectionMenuItems.getKeys().begin(), sectionInMenu)
                : 0;
        m_sectionsMenu->selected() = static_cast<int>(newSelected);
    }

    const std::string &getSelectedTag() const override {
        return m_tagMenuItems.getKeys().at(m_tagsMenu->selected());
    }

    const std::string &getSelectedSection() const override {
        return m_sectionMenuItems.getKeys().at(m_sectionsMenu->selected());
    }

  private:
    std::shared_ptr<Promptable> makeFullUIComponent();
    std::shared_ptr<WindowedMenu> makeTagsMenu();
    std::shared_ptr<WindowedMenu> makeSectionsMenu();
    CalendarOption makeCalendarOptions(const std::chrono::year_month_day &today, bool sundayStart);
};

} // namespace caps_log::view
