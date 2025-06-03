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
// TODO: rethink events, some are handled by view and some by input handler
struct OpenScratchpad {
    std::string name;
};
struct DeleteScratchpad {
    std::string name;
};
struct RenameScratchpad {
    std::string name;
};
struct UnhandledRootEvent {
    std::string input;
};

/**
 * @brief A variant type that represents all possible UI events.
 * It is used to handle different types of events in a unified way.
 */
using UIEvent = std::variant<UiStarted, DisplayedYearChange, OpenLogFile, FocusedSectionChange,
                             FocusedTagChange, FocusedDateChange, UnhandledRootEvent,
                             OpenScratchpad, DeleteScratchpad, RenameScratchpad>;

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
