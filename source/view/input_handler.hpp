#pragma once

#include <chrono>
#include <string>
#include <variant>

namespace caps_log::view {

struct UiStarted {};
struct FocusedSectionChange {};
struct FocusedTagChange {};
struct FocusedDateChange {};
struct DisplayedYearChange {
    int year;
};
struct OpenLogFile {
    std::chrono::year_month_day date{};
};
struct UnhandledRootEvent {
    std::string input;
};

/**
 * @brief A variant type that represents all possible UI events.
 * It is used to handle different types of events in a unified way.
 */
using UIEvent = std::variant<UiStarted, DisplayedYearChange, OpenLogFile, FocusedSectionChange,
                             FocusedTagChange, FocusedDateChange, UnhandledRootEvent>;

/**
 * @brief A base class for handling input events in the application.
 */
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
