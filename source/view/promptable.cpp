#include "promptable.hpp"
#include <cassert>
#include <ftxui/dom/elements.hpp>

namespace caps_log::view {

constexpr std::size_t indexYesNot = 1;
constexpr std::size_t indexOk = 2;
constexpr std::size_t indexSpinner = 3;

Promptable::Promptable(Component main) : m_main(std::move(main)) {
    auto buttons = Container::Horizontal({
        Button("Yes",
               [this] {
                   m_callback();
                   m_depth = 0;
               }),
        Button("No", [this] { m_depth = 0; }),
    });

    auto okButton = Container::Horizontal({
        Button("Ok",
               [this] {
                   m_callback();
                   m_depth = 0;
               }),
    });

    auto yesNoRenderer = Renderer(buttons, [this, buttons]() {
        return vbox(text(m_message), separator(), buttons->Render() | center) | center | border;
    });

    auto okRenderer = Renderer(okButton, [this, okButton]() {
        return vbox(text(m_message), separator(), okButton->Render() | center) | center | border;
    });

    auto spinnerRenderer = Renderer([this] { return window(text("Loading..."), text(m_message)); });

    Components comps{m_main, yesNoRenderer, okRenderer, spinnerRenderer};

    assert(indexYesNot ==
           std::distance(comps.begin(), std::find(comps.begin(), comps.end(), yesNoRenderer)));
    assert(indexOk ==
           std::distance(comps.begin(), std::find(comps.begin(), comps.end(), okRenderer)));
    assert(indexSpinner ==
           std::distance(comps.begin(), std::find(comps.begin(), comps.end(), spinnerRenderer)));

    auto tab = Container::Tab(comps, &m_depth);
    this->m_prompt = tab;
    Add(tab);
}

void Promptable::prompt(std::string message, std::function<void()> callback) {
    m_message = std::move(message);
    m_callback = std::move(callback);
    m_depth = indexYesNot;
}

void Promptable::promptOk(std::string message, std::function<void()> callback) {
    m_message = std::move(message);
    m_callback = std::move(callback);
    m_depth = indexOk;
}

void Promptable::loadingScreen(std::string message) {
    m_message = std::move(message);
    m_callback = nullptr;
    m_depth = indexSpinner;
}

Element Promptable::Render() {
    if (m_depth == 0) {
        return m_main->Render();
    }
    return dbox(m_main->Render() | dim | color(Color::Grey37),
                m_prompt->ChildAt(m_depth)->Render() | clear_under | center);
}

} // namespace caps_log::view
