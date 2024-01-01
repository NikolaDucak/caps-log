#pragma once

#include "date/date.hpp"
#include <variant>

namespace caps_log::view {

struct UIEvent {
    enum Type {
        UI_STARTED,
        ROOT_EVENT,
        FOCUSED_DATE_CHANGE,
        FOCUSED_SECTION_CHANGE,
        FOCUSED_TAG_CHANGE,
        CALENDAR_BUTTON_CLICK
    };
    const Type type;
    const std::string input;

    UIEvent(Type type, std::string input = "") : type{type}, input{std::move(input)} {}
};

// TODO: DisplayedYearChange -> DisplayedIntervalChange
// std::variant<OpenLogEvent, FocusedDateChange, FocusedSectionChange, FocusedTagChange,
// FocusedTagChange, DisplayedYearChange, ToggleTag>

class InputHandlerBase {
  public:
    virtual ~InputHandlerBase(){};
    virtual bool handleInputEvent(const UIEvent &event) = 0;
};

} // namespace caps_log::view
