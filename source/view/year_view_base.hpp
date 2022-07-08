#pragma once

#include "input_handler.hpp"

#include <functional>
#include <string>
#include <vector>

namespace clog::view {

using namespace date;

class YearViewBase {
  public:
    virtual void run() = 0;
    virtual void stop() = 0;

    virtual void setInputHandler(InputHandlerBase *handler) = 0;

    virtual Date getFocusedDate() const = 0;

    virtual void showCalendarForYear(unsigned year) = 0;
    virtual void prompt(std::string message, std::function<void()> callback) = 0;

    virtual void setAvailableLogsMap(const YearMap<bool> *map) = 0;
    virtual void setHighlightedLogsMap(const YearMap<bool> *map) = 0;

    virtual void setTagMenuItems(std::vector<std::string> items) = 0;
    virtual void setSectionMenuItems(std::vector<std::string> items) = 0;

    virtual void setPreviewString(const std::string &string) = 0;

    virtual void withRestoredIO(std::function<void()> func) = 0;

    virtual int &selectedTag() = 0;
    virtual int &selectedSection() = 0;
};

} // namespace clog::view
