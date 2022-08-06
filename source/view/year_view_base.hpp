#pragma once

#include "input_handler.hpp"

#include <functional>
#include <string>
#include <vector>

namespace clog::view {

using namespace date;

/**
 * A utility function that formats a string for a section or tag menu with a title
 * and a number of mentions. eg '(10) tag title'
 */
inline std::string makeMenuItemTitle(std::string title, unsigned count) {
    return std::string{"("} + std::to_string(count) + ") - " + title;
}

class YearViewBase {
  public:
    virtual void run() = 0;
    virtual void stop() = 0;

    virtual void setInputHandler(InputHandlerBase *handler) = 0;

    virtual Date getFocusedDate() const = 0;

    virtual void showCalendarForYear(unsigned year) = 0;
    virtual void prompt(std::string message, std::function<void()> callback) = 0;

    // passing only a pointer and having a view have no ownership of
    // the map allows for having precoputed maps and switching
    virtual void setAvailableLogsMap(const YearMap<bool> *map) = 0;
    virtual void setHighlightedLogsMap(const YearMap<bool> *map) = 0;
    // can't use a pointer here because some FTXUI menu limitations
    virtual void setTagMenuItems(std::vector<std::string> items) = 0;
    virtual void setSectionMenuItems(std::vector<std::string> items) = 0;

    virtual void setPreviewString(const std::string &string) = 0;

    virtual void withRestoredIO(std::function<void()> func) = 0;

    virtual int &selectedTag() = 0;
    virtual int &selectedSection() = 0;
};

} // namespace clog::view
