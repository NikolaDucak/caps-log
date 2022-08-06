#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <array>
#include <map>
#include <stdexcept>
#include <string>

#include <chrono>

namespace clog::date {

enum Weekday {
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESSDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6,
    SUNDAY = 7,
};

enum Month {
    JANUARY = 1,
    FEBRUARY = 2,
    MARCH = 3,
    APRIL = 4,
    MAY = 5,
    JUNE = 6,
    JULY = 7,
    AUGUST = 8,
    SEPTEMBER = 9,
    OCTOBER = 10,
    NOVEMBER = 11,
    DECEMBER = 12,
};

struct Date {
    unsigned day, month, year;

    /**
     * A constructor that accepts values for a specific date (values are
     * starting from 1).
     */
    Date(unsigned day, unsigned month, unsigned year);

    /**
     * A constructor that accepts a date represented as 2-366.
     */
    Date(unsigned day, unsigned year);

    /**
     * Returns mon-sun as 1-7 of the date.
     */
    unsigned getWeekday() const;

    /**
     * Returns todays date.
     */
    static Date getToday();

    std::string formatToString(const std::string &format) const;

  private:
    bool isValid() const;
};

std::ostream &operator<<(std::ostream &out, const Date &d);
bool operator==(const Date &l, const Date &r);
bool operator!=(const Date &l, const Date &r);
bool operator<(const Date &l, const Date &r);

/**
 * A type of "map" container that maps dates in one year to T
 **/
template <typename T> class YearMap {
    std::array<std::array<T, 31>, 12> map {};

  public:
    T &get(const Date &date) { return map[date.month - 1][date.day - 1]; }
    const T &get(const Date &date) const { return map[date.month - 1][date.day - 1]; }
    T &get(unsigned day, unsigned month) { return map[month - 1][day - 1]; }
    const T &get(unsigned day, unsigned month) const { return map[month - 1][day - 1]; }

    void set(const Date &date, const T &value) { map[date.month - 1][date.day - 1] = value; }
    T &set(unsigned day, unsigned month, T &val) { return map[month - 1][day - 1] = val; }

    inline unsigned daysSet() const {
        unsigned result = 0;
        for (const auto &month : map) {
            result += std::count(month.begin(), month.end(), true);
        }
        return result;
    }

    inline bool hasAnyDaySet() const {
        for (const auto &month : map) {
            if (std::any_of(month.begin(), month.end(), [](auto v) { return v; })) {
                return true;
            }
        }
        return false;
    }
};

using StringYearMap = std::map<std::string, YearMap<bool>>;

unsigned getNumberOfDaysForMonth(unsigned month, unsigned year);
unsigned getStartingWeekdayForMonth(unsigned month, unsigned year);
std::string getStringNameForMonth(unsigned month);

} // namespace clog::model
