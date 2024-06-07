#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

namespace caps_log::utils::date {

namespace detail {
constexpr auto kTmYearStart = 1900;
constexpr auto kLeapYearFebruaryDays = 29;
inline std::tm dateToTm(const std::chrono::year_month_day &date) {
    std::tm time = {};
    time.tm_year = int(date.year()) - kTmYearStart;
    time.tm_mon = static_cast<int>(static_cast<unsigned>(date.month())) - 1; // tm_mon is 0-based
    time.tm_mday = static_cast<int>(static_cast<unsigned>((date.day())));    // tm_mday is 1-based
    time.tm_hour = 0;
    time.tm_min = 0;
    time.tm_sec = 0;
    time.tm_isdst = -1; // Not dealing with DST
    return time;
}

} // namespace detail

inline std::string formatToString(const std::chrono::year_month_day &date,
                                  const std::string &format = "%d. %m. %y.") {
    std::ostringstream oss;
    std::tm dateTime = detail::dateToTm(date);
    oss << std::put_time(&dateTime, format.c_str());
    return oss.str();
}

inline std::chrono::year_month_day getToday() {
    auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    return std::chrono::year_month_day{today};
}

inline bool isWeekend(const std::chrono::year_month_day &date) {
    const auto sysDays = std::chrono::sys_days{date};
    std::chrono::weekday dayOfWeek{sysDays};
    return dayOfWeek == std::chrono::Saturday || dayOfWeek == std::chrono::Sunday;
}

inline unsigned getNumberOfDaysForMonth(std::chrono::month month, std::chrono::year year) {
    static const std::array<unsigned, 13> kDays{
        0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
    };

    if (year.is_leap() && month == std::chrono::February) {
        return detail::kLeapYearFebruaryDays;
    }

    return kDays.at(static_cast<unsigned>(month));
}

inline unsigned getStartingWeekdayForMonth(std::chrono::year_month year_month) {
    std::tm firstDayOfTheMonth{};
    firstDayOfTheMonth.tm_mday = 0;
    firstDayOfTheMonth.tm_mon = static_cast<int>(static_cast<unsigned>(year_month.month())) - 1;
    firstDayOfTheMonth.tm_year = static_cast<int>(year_month.year()) - detail::kTmYearStart;

    auto time = std::mktime(&firstDayOfTheMonth);
    auto *targetTimeResult = std::localtime(&time);

    if (targetTimeResult->tm_wday == 0) {
        return 1;
    }
    return targetTimeResult->tm_wday + 1;
}

inline std::string getStringNameForMonth(std::chrono::month month) {
    assert(month.ok());
    static const std::array<std::string, 13> kMonthNames{
        "", // month 0 == error
        "January", "February", "March",     "April",   "May",      "June",
        "July",    "August",   "September", "October", "November", "December"};

    const auto index = static_cast<unsigned>(month);
    return kMonthNames.at(index);
}

/**
 * A type of "map" container that maps dates in one year to T
 **/
template <typename T> class AnnualMap {
    static constexpr auto kDaysInMonth = 31;
    static constexpr auto kMonthsInYear = 12;
    std::array<std::array<T, kDaysInMonth>, kMonthsInYear> m_map{};

  public:
    T &get(const std::chrono::year_month_day &date) {
        return m_map[static_cast<unsigned>(date.month()) - 1]
                    [static_cast<unsigned>(date.day()) - 1];
    }
    const T &get(const std::chrono::year_month_day &date) const {
        return m_map[static_cast<unsigned>(date.month()) - 1]
                    [static_cast<unsigned>(date.day()) - 1];
    }
    T &get(unsigned day, unsigned month) { return m_map[month - 1][day - 1]; }
    const T &get(unsigned day, unsigned month) const { return m_map[month - 1][day - 1]; }

    void set(const std::chrono::year_month_day &date, const T &value) {
        m_map[static_cast<unsigned>(date.month()) - 1][static_cast<unsigned>(date.day()) - 1] =
            value;
    }
    T &set(unsigned day, unsigned month, T &val) { return m_map[month - 1][day - 1] = val; }

    inline unsigned daysSet() const {
        unsigned result = 0;
        for (const auto &month : m_map) {
            result += std::count(month.begin(), month.end(), true);
        }
        return result;
    }

    inline bool hasAnyDaySet() const {
        return std::ranges::any_of(m_map, [](const auto &month) {
            return std::ranges::any_of(month, [](const auto &day) { return day; });
        });
    }
};

using StringYearMap = std::map<std::string, AnnualMap<bool>>;

} // namespace caps_log::utils::date
