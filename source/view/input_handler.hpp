#pragma once

#include "../model/date.hpp"

namespace clog::view {

struct UIEvent {
    enum Type {
        ROOT_EVENT,
        FOCUSED_DATE_CHANGE,
        FOCUSED_SECTION_CHANGE,
        FOCUSED_TAG_CHANGE,
        CALENDAR_BUTTON_CLICK
    };
    const Type type;
    const int input;

    UIEvent(Type t, int in = 0) : type { t }, input { in } {}
};

class InputHandlerBase {
public:
    virtual ~InputHandlerBase() {};
    virtual bool handleInputEvent(const UIEvent& event) = 0;
};

}  // namespace clog::view
