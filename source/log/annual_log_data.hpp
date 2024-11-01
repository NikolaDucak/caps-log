#pragma once

#include "log_repository_base.hpp"
#include "utils/date.hpp"
#include <chrono>
#include <memory>
#include <string>

namespace caps_log::log {

/**
 * A class containing superficial data about the collection of logs.
 * It contains data like which dates have log entries, what tags/sections/tasks were
 * mentioned in which log entries but not actual log contents.
 */
class AnnualLogData {
  public:
    static constexpr auto kAnySection = "<any section>";
    static constexpr auto kAnyTag = "<any tag>";

    utils::date::Dates datesWithLogs;
    std::map<std::string, std::map<std::string, utils::date::Dates>> tagsPerSection;

    std::vector<std::string> getAllSections() const {
        std::set<std::string> sections;
        for (const auto &[section, _] : tagsPerSection) {
            sections.insert(section);
        }
        // remove <any section> from the list
        sections.erase(kAnySection);
        return {sections.begin(), sections.end()};
    }

    std::vector<std::string> getAllTags() const {
        std::set<std::string> tags;
        for (const auto &[_, tagsMap] : tagsPerSection) {
            for (const auto &[tag, _] : tagsMap) {
                if (tag == kAnyTag) {
                    continue;
                }
                tags.insert(tag);
            }
        }
        // remove <any tag> from the list
        return {tags.begin(), tags.end()};
    }

    std::vector<std::string> getAllTagsForSection(const std::string &section) const {
        std::vector<std::string> tags;
        tags.reserve(tagsPerSection.at(section).size());
        for (const auto &[tag, _] : tagsPerSection.at(section)) {
            tags.push_back(tag);
        }
        // remove <any tag> from the list
        tags.erase(std::remove(tags.begin(), tags.end(), kAnyTag), tags.end());
        return tags;
    }

    AnnualLogData() { tagsPerSection[kAnySection][kAnyTag] = {}; }

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
