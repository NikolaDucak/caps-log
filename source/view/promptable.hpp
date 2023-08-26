#pragma once

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"

namespace caps_log::view {
using namespace ftxui;

/**
 * A 'prompt' wrapper around a component that is considered root of the ui.
 * On Promptable::prompt() it provides a simple 'yes/no' prompt with a message
 * and on 'yes' it executes a callback. It is insiper by the ftxui modal window
 * example from the /examples directory in ftxui repo.
 */
class Promptable : public ComponentBase {
    int m_depth = 0;
    Component m_main;
    Component m_prompt;
    std::string m_message;
    std::function<void()> m_callback;

  public:
    Promptable(Component main);
    void prompt(std::string message, std::function<void()> callback);

    Element Render() override;
};

} // namespace caps_log::view
