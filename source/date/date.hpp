#pragma once 

namespace clog::date {

class Day {
    unsigned m_day;
public:
    Day(unsigned day) : m_day() {}
};

class Month {
    unsigned m_month;
public:
    Month (unsigned month) : m_month(month) { }
};

class Year {
    unsigned m_year;
public:
};

class Date {
public:
    Date(Year year, Month month, Day day);
    inline bool ok();
};

}
