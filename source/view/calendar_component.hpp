#pragma once

#include "../model/Date.hpp"
#include "./ftxui_ext/extended_containers.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>

namespace clog::view {

using namespace ftxui;
using model::Date;

struct CalendarOption {
    std::function<Element(const Date&, const EntryState&)> transform = nullptr;
    std::function<void(const Date&)> focusChange = nullptr;
    std::function<void(const Date&)> enter = nullptr;
};

class Calendar : public ComponentBase {
    model::Date m_today;
    Component m_root;
    int m_selectedMonth = 0;
    int m_selectedDay[12] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    CalendarOption m_option;
    unsigned m_displayedYear;


public:
    Calendar(const model::Date& today, CalendarOption option = {});
    auto root() { return m_root; }

    void displayYear(unsigned year) {
        m_displayedYear = year;
        m_root->Detach();
        m_root = createYear(year);
        Add(m_root);
    }

    bool OnEvent(Event event) override {
        // there is no way to inject a callback for selection change for
        // ftxui containers as for ftxui menus, this is a workaround to
        // check for selection change and trigger a callback
        static auto lastSelectedDate = getFocusedDate();
        const auto result            = ComponentBase::OnEvent(event);
        const auto newDate           = getFocusedDate();
        if (m_option.focusChange && lastSelectedDate != newDate) {
            m_option.focusChange(newDate);
            lastSelectedDate = newDate;
        }
        return result;
    }

    // TODO: perhaps it would be nicer to get a pointer to a Date where
    // this component stores the focused Date, it would be more like ftxui
    model::Date getFocusedDate() {
        const auto activeMonth = m_selectedMonth;
        const auto activeDay   = m_selectedDay[m_selectedMonth];
        return { (unsigned int)activeDay + 1, (unsigned int)activeMonth + 1, m_displayedYear };
    }

    static inline auto make(const model::Date& today, CalendarOption option = {}) {
        return std::make_shared<Calendar>(today, option);
    }

private:
    Component createYear(unsigned year);
    Component createMonth(unsigned month, unsigned year);
    Component createDay(const model::Date& date);
};

}  // namespace clog::view
