#pragma once

#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/screen/screen.hpp>

namespace caps_log::view {

struct CalendarOption {
    std::function<ftxui::Element(const std::chrono::year_month_day &, const ftxui::EntryState &)>
        transform = nullptr;
    std::function<void(const std::chrono::year_month_day &)> focusChange = nullptr;
    std::function<void(const std::chrono::year_month_day &)> enter = nullptr;
    bool sundayStart = false;
    ftxui::BorderStyle calendarBorder;
    ftxui::BorderStyle monthBorder;
};

class Calendar : public ftxui::ComponentBase {
    std::function<ftxui::Dimensions()> m_screenSizeProvider;
    CalendarOption m_option;
    std::chrono::year_month_day m_today;
    ftxui::Component m_root;
    int m_selectedMonthComponentIdx;
    std::array<int, static_cast<unsigned>(std::chrono::December)> m_selectedDayButtonIdxMap{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    std::chrono::year m_displayedYear;

  public:
    Calendar(std::function<ftxui::Dimensions()> screenSizeProvider,
             const std::chrono::year_month_day &today, CalendarOption option = {});

    bool OnEvent(ftxui::Event event) override;
    ftxui::Element OnRender() override;

    void displayYear(std::chrono::year year);
    [[nodiscard]] std::chrono::year_month_day getFocusedDate() const;

    /**
     * @brief A utility factory method to create a shared pointer to a Calendar instance. This is
     * useful as FTXUI works with shared pointers to ComponentBase instances.
     */
    [[nodiscard]] static auto make(std::function<ftxui::Dimensions()> screenSizeProvider,
                                   const std::chrono::year_month_day &today,
                                   CalendarOption option = {}) {
        return std::make_shared<Calendar>(std::move(screenSizeProvider), today, std::move(option));
    }

  private:
    ftxui::Component createYear(std::chrono::year);
    ftxui::Component createMonth(std::chrono::year_month);
    ftxui::Component createDay(const std::chrono::year_month_day &date);
};

} // namespace caps_log::view
