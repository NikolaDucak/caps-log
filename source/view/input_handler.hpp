#pragma once

#include <chrono>
#include <string>
#include <variant>

namespace caps_log::view {

struct DisplayedYearChange {
    int year;
};
struct OpenLogFile {
    std::chrono::year_month_day date{};
};
struct FocusedSectionChange {
    int section;
};
struct FocusedTagChange {
    int tag;
};
struct FocusedDateChange {
    std::chrono::year_month_day date{};
};
struct UnhandledRootEvent {
    std::string input;
};
struct UiStarted {};

using UIEvent = std::variant<UiStarted, DisplayedYearChange, OpenLogFile, FocusedSectionChange,
                             FocusedTagChange, FocusedDateChange, UnhandledRootEvent>;

class InputHandlerBase {
  public:
    InputHandlerBase() = default;
    InputHandlerBase(const InputHandlerBase &) = default;
    InputHandlerBase(InputHandlerBase &&) = default;
    InputHandlerBase &operator=(const InputHandlerBase &) = default;
    InputHandlerBase &operator=(InputHandlerBase &&) = default;
    virtual ~InputHandlerBase() = default;

    virtual bool handleInputEvent(const UIEvent &event) = 0;
};

} // namespace caps_log::view
