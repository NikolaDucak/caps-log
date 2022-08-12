#include "date.hpp"

#include <array>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace clog::date {

namespace {

std::tm dateToTm(const Date &d) {
    std::tm t{};
    t.tm_mon = d.month - 1;
    t.tm_mday = static_cast<int>(d.day);
    t.tm_year = d.year - 1900;
    return t;
}

} // namespace

Date::Date(unsigned day, unsigned month, unsigned year) : day(day), month(month), year(year) {
    if (not isValid()) {
        std::ostringstream oss;
        oss << "Invalid date: " << *this;
        throw std::invalid_argument{oss.str()};
    }
}

Date::Date(unsigned day, unsigned year) {
    std::tm t = dateToTm(*this);
    t.tm_mon = 0;

    auto time = std::mktime(&t);
    auto *local_time = std::localtime(&time);

    this->day = local_time->tm_mday;
    this->month = local_time->tm_mon + 1;
    this->year = local_time->tm_year + 1900;
}

bool Date::isValid() const {
    std::tm t = dateToTm(*this);
    std::tm orig_t = t;
    auto t2 = std::mktime(&t);
    auto *full_circle_t = std::localtime(&t2);

    return full_circle_t->tm_mon == orig_t.tm_mon && full_circle_t->tm_mday == orig_t.tm_mday &&
           full_circle_t->tm_year == orig_t.tm_year;
}

unsigned Date::getWeekday() const {
    std::tm date_time = dateToTm(*this);
    auto t2 = std::mktime(&date_time);
    auto *target_time_result = std::localtime(&t2);

    // sunday = 0, barbaric
    return target_time_result->tm_wday == 0 ? 7 : target_time_result->tm_wday;
}

bool Date::isWeekend() const {
    const auto weekday = getWeekday();
    return weekday == SATURDAY || weekday == SUNDAY;
}

std::string Date::formatToString(const std::string &format) const {
    std::ostringstream a;
    std::tm date_time = dateToTm(*this);
    a << std::put_time(&date_time, format.c_str());
    return a.str();
}

Date Date::getToday() {
    time_t t = std::time(0);
    auto *time = std::localtime(&t);
    return {static_cast<unsigned>(time->tm_mday), static_cast<unsigned>(time->tm_mon + 1),
            static_cast<unsigned>(time->tm_year + 1900)};
}

std::ostream &operator<<(std::ostream &out, const Date &d) {
    out << "Date{ " << d.day << ", " << d.month << ", " << d.year << " }";
    return out;
}

bool operator==(const Date &l, const Date &r) {
    return l.year == r.year && l.month == r.month && l.day == r.day;
}

bool operator!=(const Date &l, const Date &r) { return !(l == r); }

bool operator<(const Date &l, const Date &r) {
    std::tm tl = dateToTm(l);
    std::tm tr = dateToTm(r);
    auto time_r = std::mktime(&tr);
    auto time_l = std::mktime(&tl);

    return time_l < time_r;
}

unsigned getNumberOfDaysForMonth(unsigned month, unsigned year) {
    static const std::array<unsigned, 13> days{
        0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
    };

    if (year % 4 == 0 && month == 2)
        return 29;
    else
        return days[month];
}

unsigned getStartingWeekdayForMonth(unsigned month, unsigned year) {
    std::tm first_day_of_the_month{};
    first_day_of_the_month.tm_mday = 0;
    first_day_of_the_month.tm_mon = month - 1;
    first_day_of_the_month.tm_year = year - 1900;

    auto t2 = std::mktime(&first_day_of_the_month);
    auto *target_time_result = std::localtime(&t2);

    if (target_time_result->tm_wday == 0) {
        return 1;
    }
    return target_time_result->tm_wday + 1;
}

std::string getStringNameForMonth(unsigned month) {
    static const std::array<std::string, 13> month_names{
        "", // month 0 == error
        "january", "february", "march",     "april",   "may",      "june",
        "jully",   "august",   "september", "october", "november", "december"};

    return month_names.at(month);
}

} // namespace clog::date
