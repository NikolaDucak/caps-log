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
 * Collection of information regarding the overview
 */
struct YearLogEntryData {
    YearMap<bool> logAvailabilityMap;
    StringYearMap sectionMap, taskMap, tagMap;
};

/*
 * Only class that actualy interacts with physical files on the drive
 */
class LogRepositoryBase {
    friend class LogEntryFile;

  public:
    virtual ~LogRepositoryBase() {}

    virtual YearLogEntryData collectDataForYear(unsigned year) = 0;
    virtual void injectDataForDate(YearLogEntryData &data, const Date &date) = 0;
    virtual std::optional<LogFile> readLogFile(const Date &date) = 0;
    virtual LogFile readOrMakeLogFile(const Date &date) = 0;
    virtual void removeLog(const Date &date) = 0;
    virtual std::string path(const Date &date) = 0;

    // virtual exists(const Date&);
    // virtual std::optional<LogEntry> read(const Date&);
    // virtual void write(LogEntry&);
    // virtaul void delete(const Date&);
    //
    // virtual YearOverviewData collectOverviewDataForYear();
    // virtual injectOverviewDataForDate(YearOverviewData&);
    //
    // std::string path()
};

} // namespace clog::model
