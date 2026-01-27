#pragma once

#include "utils/date.hpp"
#include "view/view_layout_base.hpp"

#include <chrono>
#include <ftxui/component/task.hpp>
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
        if (m_displayTexts.size() != m_keys.size()) {
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

class AnnualViewLayoutBase : public ViewLayoutBase {
  public:
    [[nodiscard]] virtual std::chrono::year_month_day getFocusedDate() const = 0;
    virtual void showCalendarForYear(std::chrono::year year) = 0;

    // passing only a pointer and having a view have no ownership of
    // the map allows for having precomputed maps and switching
    virtual void setDatesWithLogs(const utils::date::Dates *map) = 0;
    virtual void setHighlightedDates(const utils::date::Dates *map) = 0;
    virtual void setEventDates(const CalendarEvents *map) {};
    virtual void setPreviewString(const std::string &title, const std::string &string) = 0;

    virtual void setSelectedTag(std::string tag) = 0;
    virtual void setSelectedSection(std::string section) = 0;
    [[nodiscard]] virtual const std::string &getSelectedTag() const = 0;
    [[nodiscard]] virtual const std::string &getSelectedSection() const = 0;
    virtual MenuItems &tagMenuItems() = 0;
    virtual MenuItems &sectionMenuItems() = 0;
};

} // namespace caps_log::view
