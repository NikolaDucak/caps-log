#pragma once

#include "input_handler.hpp"
#include "utils/date.hpp"

#include <chrono>
#include <ftxui/component/task.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace caps_log::view {

/**
 * A utility function that formats a string for a section or tag menu with a title
 * and a number of mentions. eg '(10) tag title'
 */
inline std::string makeMenuItemTitle(const std::string &title, unsigned count) {
    std::stringstream sstream;
    sstream << std::setw(3) << std::setfill(' ') << count << " │ " << title;
    return sstream.str();
}

class MenuItems {
  public:
    MenuItems() = default;

    MenuItems(std::vector<std::string> displayTexts, std::vector<std::string> keys)
        : m_displayTexts{std::move(displayTexts)}, m_keys{std::move(keys)} {
        if (displayTexts.size() != keys.size()) {
            throw std::invalid_argument(
                "MenuItems: displayTexts and keys must have the same size.");
        }
    }

    [[nodiscard]] auto size() const { return m_displayTexts.size(); }

    [[nodiscard]] const std::vector<std::string> &getDisplayTexts() const { return m_displayTexts; }
    [[nodiscard]] const std::vector<std::string> &getKeys() const { return m_keys; }

  private:
    std::vector<std::string> m_displayTexts;
    std::vector<std::string> m_keys;
};

struct CalendarEvent {
    std::string name;
    std::chrono::month_day date;
    auto operator<=>(const CalendarEvent &other) const = default;
};

using CalendarEvents = std::map<std::string, std::set<CalendarEvent>>;

class AnnualViewBase { // NOLINT
  public:
    virtual ~AnnualViewBase() = default;

    virtual void run() = 0;
    virtual void stop() = 0;

    virtual void setInputHandler(InputHandlerBase *handler) = 0;

    [[nodiscard]] virtual std::chrono::year_month_day getFocusedDate() const = 0;
    virtual void showCalendarForYear(std::chrono::year year) = 0;

    virtual void post(ftxui::Task) = 0;

    virtual void prompt(std::string message, std::function<void()> callback) = 0;
    virtual void promptOk(std::string message, std::function<void()> callback) = 0;
    virtual void loadingScreen(const std::string &str) = 0;
    virtual void loadingScreenOff() = 0;

    // passing only a pointer and having a view have no ownership of
    // the map allows for having precomputed maps and switching
    virtual void setDatesWithLogs(const utils::date::Dates *map) = 0;
    virtual void setHighlightedDates(const utils::date::Dates *map) = 0;
    virtual void setEventDates(const CalendarEvents *map) {};

    // can't use a pointer here because some FTXUI menu limitations
    virtual MenuItems &tagMenuItems() = 0;
    virtual MenuItems &sectionMenuItems() = 0;

    virtual void setPreviewString(const std::string &title, const std::string &string) = 0;

    virtual void withRestoredIO(std::function<void()> func) = 0;

    virtual void setSelectedTag(std::string tag) = 0;
    virtual void setSelectedSection(std::string section) = 0;

    [[nodiscard]] virtual const std::string &getSelectedTag() const = 0;
    [[nodiscard]] virtual const std::string &getSelectedSection() const = 0;
};

} // namespace caps_log::view
