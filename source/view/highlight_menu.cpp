#include "highlight_menu.hpp"

namespace clog::view {

static model::YearMap<bool> nothing {};

CalendarHighlighMenuComponent::CalendarHighlighMenuComponent(model::StringYearMap& map,
                                                             model::YearMap<bool>*& selected) :
    m_info(map),
    m_selectedOutput(selected), m_title("test") {
    dataChanged();
}

Component CalendarHighlighMenuComponent::createFTXUIMenu() {
    MenuOption menu_options { .on_change = [this]() {
        auto item = m_info.find(m_items.second[m_selected]);
        if (item != m_info.end()) {
            m_selectedOutput = &item->second;
        } else {
            m_selectedOutput = &nothing;
        }
    } };

    auto menu          = Menu(&m_items.first, &m_selected, menu_options);
    auto menu_renderer = Renderer(
        menu, [=]() { return window(text(m_title), menu->Render()) | size(WIDTH, LESS_THAN, 25); });

    return menu_renderer;
}

void CalendarHighlighMenuComponent::dataChanged() {
    if (m_menu)
        m_menu->Detach();
    m_selectedOutput = &nothing;
    m_selected       = 0;
    m_items          = {};
    m_items.first.push_back(" -");
    m_items.second.push_back(" -");
    for (const auto& kv : m_info) {
        m_items.first.push_back("(" + std::to_string(kv.second.daysSet()) + ") " + kv.first);
        m_items.second.push_back(kv.first);
    }
    m_menu = createFTXUIMenu();
    this->Add(m_menu);
}
}  // namespace clog::view
