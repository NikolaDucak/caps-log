#pragma once

#include "calendar_component.hpp"
#include "date/date.hpp"
#include "input_handler.hpp"
#include "preview.hpp"
#include "promptable.hpp"
#include "windowed_menu.hpp"
#include "year_view_base.hpp"

#include <array>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/task.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>

namespace caps_log::view {

using namespace ftxui;

using namespace date;

class YearView : public YearViewBase {
    InputHandlerBase *m_handler;
    ScreenInteractive m_screen;

    // UI compontents visible to the user
    std::shared_ptr<Calendar> m_calendarButtons;
    std::shared_ptr<WindowedMenu> m_tagsMenu, m_sectionsMenu;
    std::shared_ptr<Preview> m_preview = std::make_unique<Preview>();
    std::shared_ptr<Promptable> m_rootComponent;

    // Maps that help m_calendarButtons highlight certain logs.
    const YearMap<bool> *m_highlightedLogsMap = nullptr;
    const YearMap<bool> *m_availabeLogsMap = nullptr;

    // Menu items for m_tagsMenu & m_sectionsMenu
    std::vector<std::string> m_tagMenuItems, m_sectionMenuItems;

  public:
    YearView(const Date &today, bool sundayStart);

    void run() override;
    void stop() override;

    void post(Task task) override;

    void prompt(std::string message, std::function<void()> onYesCallback) override;
    void promptOk(std::string message, std::function<void()> callback) override;
    void loadingScreen(const std::string &message) override;
    void loadingScreenOff() override;

    void showCalendarForYear(unsigned year) override;

    int &selectedTag() override { return m_tagsMenu->selected(); }
    int &selectedSection() override { return m_sectionsMenu->selected(); }

    void setInputHandler(InputHandlerBase *handler) override { m_handler = handler; }

    void setAvailableLogsMap(const YearMap<bool> *map) override { m_availabeLogsMap = map; }
    void setHighlightedLogsMap(const YearMap<bool> *map) override { m_highlightedLogsMap = map; }

    void setTagMenuItems(std::vector<std::string> items) override {
        m_tagMenuItems = std::move(items);
    }
    void setSectionMenuItems(std::vector<std::string> items) override {
        m_sectionMenuItems = std::move(items);
    }

    void setPreviewString(const std::string &string) override { m_preview->setContent(string); }

    void withRestoredIO(std::function<void()> func) override { m_screen.WithRestoredIO(func)(); }

    Date getFocusedDate() const override { return m_calendarButtons->getFocusedDate(); }

  private:
    std::shared_ptr<Promptable> makeFullUIComponent();
    std::shared_ptr<WindowedMenu> makeTagsMenu();
    std::shared_ptr<WindowedMenu> makeSectionsMenu();
    CalendarOption makeCalendarOptions(const Date &today, bool sundayStart);
};

} // namespace caps_log::view
