#pragma once

#include "log_repository_base.hpp"
#include "utils/date.hpp"
#include <chrono>
#include <memory>
#include <set>
#include <string>

namespace caps_log::log {

/**
 * A class containing superficial data about the collection of logs.
 * It contains data like which dates have log entries, what tags/sections/tasks were
 * mentioned in which log entries but not actual log contents.
 */
class AnnualLogData {
  public:
    utils::date::Dates datesWithLogs;
    std::map<std::string, utils::date::Dates> datesWithSection;
    std::map<std::string, utils::date::Dates> datesWithTag;

    /**
     * Constructs YearOverviewData from logs in a given year.
     */
    static AnnualLogData collect(const std::shared_ptr<LogRepositoryBase> &repo,
                                 std::chrono::year year, bool skipFirstLine = true);

    /**
     * Injects/updates the current object with information parsed from a log entry form a specified
     * date. It will remove/add it to logAvailabilityMap if deleted/written etc.
     */
    void collect(const std::shared_ptr<LogRepositoryBase> &repo,
                 const std::chrono::year_month_day &date, bool skipFirstLine = true);
};

} // namespace caps_log::log
