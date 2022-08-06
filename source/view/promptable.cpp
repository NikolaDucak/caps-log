#include "promptable.hpp"

namespace clog::view {

Promptable::Promptable(Component main) : m_main(std::move(main)) {
    auto buttons = Container::Horizontal({
        Button("yes",
               [this] {
                   m_callback();
                   m_depth = 0;
               }),
        Button("no", [this] { m_depth = 0; }),
    });
    m_prompt = Renderer(buttons, [this, buttons]() {
        return vbox(text(m_message), separator(), buttons->Render()) | center;
    });
    auto tab = Container::Tab({m_main, m_prompt}, &m_depth);
    Add(tab);
}

void Promptable::prompt(std::string message, std::function<void()> callback) {
    m_message = std::move(message);
    m_callback = std::move(callback);
    m_depth = 1;
}

Element Promptable::Render() { return (m_depth == 0) ? m_main->Render() : m_prompt->Render(); }

} // namespace clog::view
