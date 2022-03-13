#pragma once

#include <array>
#include <map>
#include <memory>
#include <algorithm>
#include <numeric>

#include "Date.hpp"

namespace clog::model {

/**
 * A some type of "map" container that maps a date in one year to T
 * @TODO: check if date belongs to a map
 **/
template <typename T>
class YearMap {
    std::array<std::array<T, 31>, 12> map;

public:
    T& get(const Date& date) {
        return map[date.month-1][date.day-1];
    }

    const T& get(const Date& date) const {
        return map[date.month-1][date.day-1];
    }

    void set(const Date& date, const T& value) {
        map[date.month-1][date.day-1] = value;
    }

    T& get(unsigned day, unsigned month) {
        return map[month-1][day-1];
    }

    const T& get(unsigned day, unsigned month) const {
        return map[month-1][day-1];
    }

    T& set(unsigned day, unsigned month, T& val) {
        return map[month-1][day-1] = val;
    }

    unsigned daysSet() const {
        unsigned result = 0;
        for(const auto& month: map) {
            result += std::count(month.begin(), month.end(), true);
        }
        return result;
    }

    bool hasAnyDaySet() const {
        for(const auto& month: map) {
           if(std::any_of(month.begin(), month.end(), [](auto v) { return v; } )) {
               return true;
           }
        }
        return false;
    }
    
};


using StringYearMap = std::map<std::string, YearMap<bool>>;

struct YearLogEntryData {
    YearMap<bool> logAvailabilityMap;
    StringYearMap sectionMap, taskMap, tagMap;
};


/**
 * Only class that actualy interacts with physical files on the drive
 */
class LogRepository {
    friend class LogEntryFile;
public:

    virtual ~LogRepository() {}

    virtual YearLogEntryData collectDataForYear(unsigned year) = 0;
    virtual void injectDataForDate(YearLogEntryData& data, const Date& date) = 0;
    virtual std::string getLogEntryPath(const Date& date) = 0;

private:
    virtual std::string readFile(const Date& date) = 0;
    virtual void removeFile(const Date& date) = 0;
};

}
