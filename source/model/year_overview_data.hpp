#pragma once

#include "log_repository_base.hpp"

namespace clog::model {

/**
 * A class containing superficial data about the collection of logs.
 * It contains data like which dates have log entries, what tags/sections/tasks were
 * mentioned in which log entries but not actual log contents.
 */
class YearOverviewData {
  public:
    date::YearMap<bool> logAvailabilityMap;
    date::StringYearMap sectionMap, tagMap;

    /**
     * Constructs YearOverviewData from logs in a given year.
     */
    static YearOverviewData collect(const std::shared_ptr<LogRepositoryBase> &repo, unsigned year,
                                    bool skipFirstLine = true);

    /**
     * Injects/updates the current object with information parsed from a log entry form a specified
     * date. It will remove/add it to logAvailabilityMap if deleted/written etc.
     */
    void collect(const std::shared_ptr<LogRepositoryBase> &repo, const date::Date &date,
                 bool skipFirstLine = true);
};

} // namespace clog::model
