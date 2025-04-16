#include "calendar_component.hpp"

#include "ftxui/dom/elements.hpp"
#include "utils/date.hpp"
#include "view/ftxui_ext/extended_containers.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <memory>

namespace caps_log::view {
using namespace ftxui;

namespace {
/**
 * @brief Utility class that allows for the arrangement of month components in a calendar.
 * It distributes 12 months into 2 or more rows of equal elements.
 * @Note This class had to be implemented to allow for fetching the screen size at runtime to
 * avoid relying on "Terminal::GetDimensions()" which is troubling in test environment.
 * @Note the class makes some assumptions about the width of the rendered month components. Like
 * that month component day buttons have a border and such
 */
class MonthComponentArranger {
  public:
    static Element arrange(const Dimensions screenDimensions, const Components &monthComponents,
                           std::chrono::year displayedYear) {
        const auto availableMonthColumns = computeNumberOfMonthsPerRow(screenDimensions);
        const auto renderData = arrangeMonthsInCalendar(monthComponents, availableMonthColumns);
        return window(text(std::to_string(static_cast<int>(displayedYear))),
                      vbox(renderData) | frame) |
               vscroll_indicator;
    }

  private:
    static constexpr auto kMaxMonthComponentsPerRow = 6;
    static constexpr auto kCharsPerDay = 4;
    static constexpr auto kDaysPerWeek = 7;
    static constexpr auto kRenderedMonthPaddingAndBorderChars = 6;
    static constexpr auto kCharsPerMonthComponent =
        kCharsPerDay * kDaysPerWeek + kRenderedMonthPaddingAndBorderChars;

    /**
     * @brief Computes the number of months that can be displayed in a row.
     */
    static int computeNumberOfMonthsPerRow(const Dimensions &screenDimensions) {
        int availableMonthColumns = screenDimensions.dimx / kCharsPerMonthComponent;
        // prevent splitting 12 months into 2 rows of unequal elements
        if (availableMonthColumns == 5) { // NOLINT
            availableMonthColumns = 4;
        }
        return std::min(kMaxMonthComponentsPerRow, availableMonthColumns);
    }

    /**
     * @brief Arranges month components in a calendar. It distributes 12 months into 2 or more rows
     * of equal elements.
     */
    static Elements arrangeMonthsInCalendar(const Components &monthComponents, int columns) {
        Elements renderData;
        Elements currentHbox;
        for (auto i = 0; i < monthComponents.size(); i++) {
            const auto &month = monthComponents[i];
            currentHbox.push_back(month->Render());
            if ((i + 1) % columns == 0) {
                renderData.push_back(hbox(currentHbox));
                currentHbox.clear();
            }
        }
        return renderData;
    }
};

} // namespace

Calendar::Calendar(std::unique_ptr<ScreenSizeProvider> screenSizeProvider,
                   const std::chrono::year_month_day &today, CalendarOption option)
    : m_screenSizeProvider{std::move(screenSizeProvider)}, m_option(std::move(option)),
      m_today(today),
      // TODO: SetActiveChild does nothing, this is the only
      // way to focus a specific date on startup. Investigate.
      m_selectedMonthComponentIdx{(static_cast<int>(static_cast<unsigned>(today.month()))) - 1},
      m_root{createYear(m_today.year())}, m_displayedYear{m_today.year()} {
    Add(m_root);
    // the rest will selfcorrect
    m_selectedDayButtonIdxMap.at(m_selectedMonthComponentIdx) =
        static_cast<int>(static_cast<unsigned>(today.day()) - 1U);
}

void Calendar::displayYear(std::chrono::year year) {
    m_displayedYear = year;
    m_root->Detach();
    m_root = createYear(year);
    Add(m_root);
}

bool Calendar::OnEvent(ftxui::Event event) {
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

ftxui::Element Calendar::Render() {
    if (Focused()) {
        return m_root->Render();
    }
    return m_root->Render() | ftxui::dim;
}

std::chrono::year_month_day Calendar::getFocusedDate() {
    using namespace std::chrono;
    return std::chrono::year_month_day(
        m_displayedYear, month{(unsigned)m_selectedMonthComponentIdx + 1},
        day{(unsigned)m_selectedDayButtonIdxMap.at(m_selectedMonthComponentIdx) + 1});
}

Component Calendar::createYear(std::chrono::year year) {
    Components monthComponents;

    // NOTE: std:: ++ operator for months loops over valid months in circle forever
    constexpr unsigned kJan = 1U;
    constexpr unsigned kDecPlusOne = 13U;
    for (auto month = kJan; month < kDecPlusOne; month++) {
        monthComponents.push_back(createMonth({year, std::chrono::month{month}}));
    }
    const auto container = ftxui_ext::AnyDir(monthComponents, &m_selectedMonthComponentIdx);

    return Renderer(container, [this, monthComponents]() {
        return MonthComponentArranger::arrange(m_screenSizeProvider->getScreenSize(),
                                               monthComponents, m_displayedYear);
    });
}

Component Calendar::createMonth(std::chrono::year_month year_month) {
    static const auto kMonthWeekdayHeaderElement = [](const std::string &txt) {
        return center(text(txt)) | size(WIDTH, EQUAL, 3) | size(HEIGHT, EQUAL, 1);
    };

    const auto numOfDays =
        std::chrono::year_month_day_last{year_month.year(),
                                         std::chrono::month_day_last{year_month.month()}}
            .day();

    Components buttons;
    for (auto day = std::chrono::day{1}; day <= numOfDays; day++) {
        buttons.push_back(
            createDay(std::chrono::year_month_day{year_month.year(), year_month.month(), day}));
    }
    static constexpr auto kDaysPerWeek = 7;
    const auto container = ftxui_ext::Grid(
        kDaysPerWeek, buttons,
        &m_selectedDayButtonIdxMap.at(static_cast<unsigned>(year_month.month()) - 1));

    return Renderer(container, [&displayedYear = m_displayedYear, yearMonth = year_month,
                                sundayStart = (m_option.sundayStart ? 1 : 0),
                                buttons = std::move(buttons)]() {
        const auto header =
            Elements{kMonthWeekdayHeaderElement("M"), kMonthWeekdayHeaderElement("T"),
                     kMonthWeekdayHeaderElement("W"), kMonthWeekdayHeaderElement("T"),
                     kMonthWeekdayHeaderElement("F"), kMonthWeekdayHeaderElement("S"),
                     kMonthWeekdayHeaderElement("S")} |
            underlined;
        std::vector<Elements> renderData = {header, {}};
        const auto startingWeekday =
            utils::date::getStartingWeekdayForMonth({displayedYear, yearMonth.month()}) +
            sundayStart;
        unsigned currentWeekday = startingWeekday - 1;
        unsigned calendarDay = 1;
        for (int i = 1; i < startingWeekday; i++) {
            renderData.back().push_back(filler());
        }
        for (const auto &btn : buttons) {
            if (currentWeekday % kDaysPerWeek == 0) {
                renderData.push_back({});
            }
            renderData.back().push_back(btn->Render());
            calendarDay++;
            currentWeekday++;
        }
        return window(text(utils::date::getStringNameForMonth(yearMonth.month())),
                      gridbox(renderData));
    });
}

Component Calendar::createDay(const std::chrono::year_month_day &date) {

    ButtonOption opts{};
    if (m_option.transform) {
        opts.transform = [this, date](const auto &state) {
            return m_option.transform(date, state);
        };
    }
    return Button(
        std::to_string(static_cast<unsigned>(date.day())),
        [this, date]() {
            if (m_option.enter) {
                m_option.enter(date);
            }
        },
        opts);
}

} // namespace caps_log::view
