#include "year_overview_data.hpp"
#include "date/date.hpp"
#include <regex>

namespace clog::model {

YearOverviewData YearOverviewData::collect(const std::shared_ptr<LogRepositoryBase> &repo,
                                           unsigned year, bool skipFirstLine) {
    YearOverviewData data;

    for (unsigned month = date::Month::JANUARY; month <= date::Month::DECEMBER; month++) {
        for (unsigned day = 1; day <= date::getNumberOfDaysForMonth(month, year); day++) {
            data.collect(repo, date::Date{day, month, year}, skipFirstLine);
        }
    }

    return std::move(data);
}

void YearOverviewData::collect(const std::shared_ptr<LogRepositoryBase> &repo,
                               const date::Date &date, bool skipFirstLine) {
    // first remove all set mentions
    utils::mapRemoveIf(tagMap, [date](auto &tag) {
        tag.second.set(date, false);
        return tag.second.hasAnyDaySet() == 0;
    });
    clog::utils::mapRemoveIf(sectionMap, [date](auto &section) {
        section.second.set(date, false);
        return section.second.hasAnyDaySet() == 0;
    });

    auto input = repo->read(date);

    // if there is no log to be processed, return
    if (not input) {
        logAvailabilityMap.set(date, false);
        return;
    }

    logAvailabilityMap.set(date, true);

    // then parse and set mentions again
    auto parsedSections = input->readSectionTitles(skipFirstLine);
    auto parsedTags = input->readTagTitles();

    for (const auto &tag : parsedTags) {
        tagMap[tag].set(date, true);
    }
    for (const auto &section : parsedSections) {
        sectionMap[section].set(date, true);
    }
}

} // namespace clog::model
