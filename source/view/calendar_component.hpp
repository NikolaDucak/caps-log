#pragma once

#include "date/date.hpp"
#include "view/ftxui_ext/extended_containers.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>

namespace clog::view {

struct CalendarOption {
    std::function<ftxui::Element(const date::Date &, const ftxui::EntryState &)> transform =
        nullptr;
    std::function<void(const date::Date &)> focusChange = nullptr;
    std::function<void(const date::Date &)> enter = nullptr;
    bool sundayStart = false;
};

class Calendar : public ftxui::ComponentBase {
    CalendarOption m_option;
    date::Date m_today;
    ftxui::Component m_root;
    int m_selectedMonth = 0;
    std::array<int, 12> m_selectedDay{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned m_displayedYear;

  public:
    Calendar(const date::Date &today, CalendarOption option = {});

    void displayYear(unsigned year) {
        m_displayedYear = year;
        m_root->Detach();
        m_root = createYear(year);
        Add(m_root);
    }

    bool OnEvent(ftxui::Event event) override {
        // there is no way to inject a callback for selection change for
        // ftxui containers as for ftxui menus, this is a workaround to
        // check for selection change and trigger a callback
        static auto lastSelectedDate = getFocusedDate();
        const auto result = ftxui::ComponentBase::OnEvent(event);
        const auto newDate = getFocusedDate();
        if (m_option.focusChange && lastSelectedDate != newDate) {
            m_option.focusChange(newDate);
            lastSelectedDate = newDate;
        }
        return result;
    }

    ftxui::Element Render() override {
        if (Focused()) 
            return m_root->Render();
        else
            return m_root->Render() | ftxui::dim;
    }

    // TODO: perhaps it would be nicer to get a pointer to a Date where
    // this component stores the focused Date, it would be more like ftxui
    date::Date getFocusedDate() {
        const auto activeMonth = m_selectedMonth;
        const auto activeDay = m_selectedDay[m_selectedMonth];
        return {(unsigned int)activeDay + 1, (unsigned int)activeMonth + 1, m_displayedYear};
    }

    static inline auto make(const date::Date &today, CalendarOption option = {}) {
        return std::make_shared<Calendar>(today, option);
    }

  private:
    ftxui::Component createYear(unsigned year);
    ftxui::Component createMonth(unsigned month, unsigned year);
    ftxui::Component createDay(const date::Date &date);
};

} // namespace clog::view
