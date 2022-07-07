#include "calendar_component.hpp"

#include "ftxui/dom/elements.hpp"


namespace clog::view {

namespace {
    Elements arrangeMonthsInCalendar(const Components& monthComponents, int columns) {
        Elements render_data;
        Elements curr_hbox;
        int i = 1;
        for (const auto& month : monthComponents) {
            curr_hbox.push_back(month->Render());
            if (i % columns == 0) {
                render_data.push_back(hbox(curr_hbox));
                curr_hbox.clear();
            }
            i++;
        }
        return render_data;
    }
}

Calendar::Calendar(const model::Date& today, CalendarOption option) :
    m_today(today),
    // TODO: SetActiveChild does nothing, this is the only
    // way to focus a specific date on startup. Investigate.
    m_selectedMonth(today.month - 1), m_root(createYear(m_today.year)), m_option(std::move(option)),
    m_displayedYear(today.year) {
    Add(m_root);
    // the rest will selfcorect
    m_selectedDay[m_selectedMonth] = today.day - 1;
}

Component Calendar::createYear(unsigned year) {
    Components month_components;
    for (unsigned month = 1; month <= 12; month++) {
        month_components.push_back(createMonth(month, year));
    }
    auto container = ftxui_ext::AnyDir(month_components, &m_selectedMonth);

    return Renderer(container, [month_components, this]() {
        int available_month_columns = Terminal::Size().dimx / (4 * 7 + 6);
        available_month_columns = std::min(6, available_month_columns);
        if (available_month_columns == 5) {
            available_month_columns = 4;
        }
        return window(text(std::to_string(m_displayedYear)), 
            vbox(arrangeMonthsInCalendar(month_components, available_month_columns)) | frame) 
            | vscroll_indicator;
    });
}

Component Calendar::createMonth(unsigned month, unsigned year) {
    static const auto cell = [](const std::string &txt) {
        return center(text(txt)) | size(WIDTH, EQUAL, 3) | size(HEIGHT, EQUAL, 1);
    };

    const auto num_of_days = model::getNumberOfDaysForMonth(month, year);

    Components buttons;
    for (unsigned day = 1; day <= num_of_days; day++) {
        buttons.push_back(createDay(model::Date{day, month, year}));
    }
    m_selectedDay[month - 1] = 0;
    const auto container     = ftxui_ext::Grid(7, buttons, &m_selectedDay[month - 1]);

    auto root_component = Renderer(container, [=, this]() {
        const Elements header1 =
            Elements{cell("M"), cell("T"), cell("W"), cell("T"), cell("F"), cell("S"), cell("S")} |
            underlined;
        std::vector<Elements> render_data = {header1, {}};
        auto starting_weekday = clog::model::getStartingWeekdayForMonth(month, m_displayedYear);
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

    return root_component;
}

Component Calendar::createDay(const model::Date &date) {
    return Button(
        std::to_string(date.day), [this, date]() { m_option.enter(date); },
        ButtonOption{.transform = [this, date](const auto &state) {
            return m_option.transform(date, state);
        }});
}

} // namespace clog::view
