#pragma once 

namespace clog::date {

class Day {
    unsigned m_day;
public:
    Day(unsigned) {}
};

class Month {
    unsigned m_month;
public:
};

class Year {
    unsigned m_year;
public:
};

class Date {
    
public:
    inline bool ok();
};


}
