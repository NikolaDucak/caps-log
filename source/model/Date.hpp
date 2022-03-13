#pragma once

#include <string>
#include <vector>

namespace clog::model {

struct Date {
    unsigned day, month, year;

    enum Month {
        JANUARY   = 1,
        FEBRUARY  = 2,
        MARCH     = 3,
        APRIL     = 4,
        MAY       = 5,
        JUNE      = 6,
        JULLY     = 7,
        AUGUST    = 8,
        SEPTEMBER = 9,
        OCTOBER   = 10,
        NOVEMBER  = 11,
        DECEMBER  = 12,
    };

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

    std::string formatToString(const std::string& format) const;

private:
    bool isValid() const;
};

std::ostream& operator<<(std::ostream& out, const Date& d);
bool operator==(const Date& l, const Date& r);
bool operator<(const Date& l, const Date& r);

unsigned getNumberOfDaysForMonth(unsigned month, unsigned year);
unsigned getStartingWeekdayForMonth(unsigned month, unsigned year);
std::string getStringNameForMonth(unsigned month);

}  // namespace clog::model
