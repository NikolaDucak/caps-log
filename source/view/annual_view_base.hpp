#pragma once

#include "input_handler.hpp"
#include "utils/date.hpp"

#include <chrono>
#include <ftxui/component/task.hpp>
#include <functional>
#include <string>
#include <vector>

namespace caps_log::view {

/**
 * A utility function that formats a string for a section or tag menu with a title
 * and a number of mentions. eg '(10) tag title'
 */
inline std::string makeMenuItemTitle(const std::string &title, unsigned count) {
    return std::string{"("} + std::to_string(count) + ") - " + title;
}

class AnnualViewBase { // NOLINT
  public:
    virtual ~AnnualViewBase() = default;

    virtual void run() = 0;
    virtual void stop() = 0;

    virtual void setInputHandler(InputHandlerBase *handler) = 0;

    virtual std::chrono::year_month_day getFocusedDate() const = 0;
    virtual void showCalendarForYear(std::chrono::year year) = 0;

    virtual void post(ftxui::Task) = 0;

    virtual void prompt(std::string message, std::function<void()> callback) = 0;
    virtual void promptOk(std::string message, std::function<void()> callback) = 0;
    virtual void loadingScreen(const std::string &str) = 0;
    virtual void loadingScreenOff() = 0;

    // passing only a pointer and having a view have no ownership of
    // the map allows for having precomputed maps and switching them out on the fly
    virtual void setDatesWithLogs(const utils::date::Dates *map) = 0;
    virtual void setHighlightedDates(const utils::date::Dates *map) = 0;

    // can't use a pointer here because some FTXUI menu limitations
    virtual void setTagMenuItems(std::vector<std::string> items) = 0;
    virtual void setSectionMenuItems(std::vector<std::string> items) = 0;

    virtual void setPreviewString(const std::string &string) = 0;

    virtual void withRestoredIO(std::function<void()> func) = 0;

    virtual int &selectedTag() = 0;
    virtual int &selectedSection() = 0;
};

} // namespace caps_log::view
