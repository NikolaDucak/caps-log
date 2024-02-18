#pragma once

#include <string>

namespace caps_log::view {

struct UIEvent {
    enum Type {
        UiStarted,
        RootEvent,
        FocusedDateChange,
        FocusedSectionChange,
        FocusedTagChange,
        CalendarButtonClick
    };
    Type type;
    std::string input;

    UIEvent(Type type, std::string input = "") : type{type}, input{std::move(input)} {}
};

// TODO: DisplayedYearChange -> DisplayedIntervalChange
// std::variant<OpenLogEvent, FocusedDateChange, FocusedSectionChange, FocusedTagChange,
// FocusedTagChange, DisplayedYearChange, ToggleTag>

class InputHandlerBase { // NOLINT
  public:
    virtual ~InputHandlerBase() = default;
    virtual bool handleInputEvent(const UIEvent &event) = 0;
};

} // namespace caps_log::view
