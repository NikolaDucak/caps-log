#pragma once

#include <functional>
#include <vector>
#include <string>

#include "input_handler.hpp"

namespace clog::view {

class YearViewBase {
public:
    virtual void run() = 0;
    virtual void stop() = 0;

    virtual void setInputHandler(InputHandlerBase* handler) = 0;

    virtual model::Date getFocusedDate() const = 0;

    virtual void showCalendarForYear(unsigned year) = 0;
    virtual void prompt(std::string message, std::function<void()> callback) = 0;

    virtual void setAvailableLogsMap(const model::YearMap<bool>* map) = 0;
    virtual void setHighlightedLogsMap(const model::YearMap<bool>* map) = 0;

    virtual void setTagMenuItems(std::vector<std::string> items) = 0;
    virtual void setSectionMenuItems(std::vector<std::string> items) = 0;

    virtual void setPreviewString(const std::string& string)= 0;
    
    virtual void withRestoredIO(std::function<void()> func) = 0;

    virtual int& selectedTag() = 0; 
    virtual int& selectedSection()= 0;

};

}
