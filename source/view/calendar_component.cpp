#include "calendar_component.hpp"

namespace clog::view {

CalendarComponent::CalendarComponent(const model::Date& today,
                                     const model::YearMap<bool>* datesWithLogEntries,
                                     std::function<void(const model::Date&)> onEnterCallback) :
    m_today(today),
    m_root(createYear(m_today.year)), m_logAvailabilityMap(datesWithLogEntries),
    m_onEnterCallback(onEnterCallback) {}

Component CalendarComponent::createYear(unsigned year) {
    Components month_components;

    for (unsigned month = 1; month <= 12; month++) {
        month_components.push_back(createMonth(month, year));
    }
    auto container = ftxui_ext::AnyDir(month_components);
    container->SetActiveChild(month_components.at(m_today.month - 1));

    return Renderer(container, [=]() {
        int available_month_columns = Terminal::Size().dimx / (4 * 7 + 6);
        available_month_columns     = std::min(6, available_month_columns);
        if (available_month_columns == 5) {
            available_month_columns = 4;
        }

        Elements render_data;
        Elements curr_hbox;
        int i = 1;
        for (const auto& month : month_components) {
            curr_hbox.push_back(month->Render());
            if (i % available_month_columns == 0) {
                render_data.push_back(hbox(curr_hbox));
                curr_hbox.clear();
            }
            i++;
        }
        return window(text("Calendar"), vbox(render_data) | vscroll_indicator | frame);
    });
}

Component CalendarComponent::createMonth(unsigned month, unsigned year) {
    static ButtonOption o { false };
    static const auto cell = [](const std::string& txt) {
        return center(text(txt)) | size(WIDTH, EQUAL, 3) | size(HEIGHT, EQUAL, 1);
    };

    const auto num_of_days = model::getNumberOfDaysForMonth(month, year);
    Components buttons;

    for (unsigned day = 1; day <= num_of_days; day++) {
        buttons.push_back(createDay(model::Date { day, month, year }));
    }

    auto container = ftxui_ext::Grid(7, buttons);

    auto root_component = Renderer(container, [=]() {
        const Elements header1 = Elements { cell("M"), cell("T"), cell("W"), cell("T"),
                                            cell("F"), cell("S"), cell("S") } |
                                 underlined;
        std::vector<Elements> render_data = { header1, {} };
        auto starting_weekday   = clog::model::getStartingWeekdayForMonth(month, m_today.year);
        unsigned curren_weekday = starting_weekday - 1, calendar_day = 1;
        for (int i = 1; i < starting_weekday; i++) {
            render_data.back().push_back(filler());
        }
        for (const auto btn : buttons) {
            if (curren_weekday % 7 == 0) {
                render_data.push_back({});
            }
            render_data.back().push_back(btn->Render());
            calendar_day++;
            curren_weekday++;
        }
        return window(text(model::getStringNameForMonth(month)), gridbox(render_data));
    });
    if (m_today.month == month) {
        container->SetActiveChild(buttons.at(m_today.day - 1));
    }

    return root_component;
}

Component CalendarComponent::createDay(const model::Date& date) {
    static const ButtonOption noBorder { .border = false };

    auto btn = Button(
        std::to_string(date.day), [this, date]() { m_onEnterCallback(date); }, noBorder);
    return Renderer(btn, [&, this, btn, date]() {
        auto element = btn->Render() | ::ftxui::center;
        if (m_today == date)
            element = element | underlined;
        if (m_selectedHighlightMap && m_selectedHighlightMap->get(date))
            element = element | color(Color::Yellow);
        if (m_logAvailabilityMap && not m_logAvailabilityMap->get(date))
            element = element | dim;
        return element;
    });
}

Component CalendarComponent::getFTXUIComponent() { return m_root; }

}  // namespace clog::view
