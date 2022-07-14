#pragma once

#include "date/date.hpp"
#include "log_file.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <numeric>

namespace clog::model {

/*
 * Collection of information regarding the overview. 
 */
struct YearOverviewData {
    YearMap<bool> logAvailabilityMap;
    StringYearMap sectionMap, taskMap, tagMap;
};

/*
 * Only class that actualy interacts with physical files on the drive
 */
class LogRepositoryBase {
public:
    virtual ~LogRepositoryBase() {}

    virtual YearOverviewData collectYearOverviewData(unsigned year) const = 0;
    virtual void injectOverviewDataForDate(YearOverviewData &data, const Date &date) const = 0;

    virtual std::optional<LogFile> read(const Date &date) const = 0;
    virtual void remove(const Date &date) = 0;
    virtual void write(const LogFile& log) = 0;

    virtual std::string path(const Date &date) = 0;
};

} // namespace clog::model
