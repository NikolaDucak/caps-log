#include "annual_log_data.hpp"
#include "utils/date.hpp"

namespace caps_log::log {

using utils::date::monthDay;

namespace {
void collectEmpty(AnnualLogData &data, const std::shared_ptr<LogRepositoryBase> &repo,
                  std::chrono::year_month_day date, bool skipFirstLine) {
    auto input = repo->read(date);

    // if there is no log to be processed, return
    if (not input) {
        data.datesWithLogs.erase(monthDay(date));
        return;
    }
    input->parse(skipFirstLine);

    data.datesWithLogs.insert(monthDay(date));

    const auto monthDayDate = monthDay(date);
    for (const auto &[section, tags] : input->getTagsPerSection()) {
        data.tagsPerSection[section][AnnualLogData::kAnyOrNoTag].insert(monthDayDate);
        data.tagsPerSection[AnnualLogData::kAnySection][AnnualLogData::kAnyOrNoTag].insert(
            monthDayDate);
        for (const auto &tag : tags) {
            data.tagsPerSection[section][tag].insert(monthDayDate);
            data.tagsPerSection[AnnualLogData::kAnySection][tag].insert(monthDayDate);
        }
    }
}

} // namespace

AnnualLogData AnnualLogData::collect(const std::shared_ptr<LogRepositoryBase> &repo,
                                     std::chrono::year year, bool skipFirstLine) {
    AnnualLogData data;

    constexpr auto kDecemberIndex = 12U;
    constexpr auto kJanuaryIndex = 1U;
    for (auto monthIndex = kJanuaryIndex; monthIndex <= kDecemberIndex; monthIndex++) {
        const auto month = std::chrono::month{monthIndex};
        const auto lastDay =
            std::chrono::year_month_day_last{year, std::chrono::month_day_last{month}};
        for (std::chrono::day day{1}; day <= lastDay.day(); day++) {
            collectEmpty(data, repo, std::chrono::year_month_day{year, month, day}, skipFirstLine);
        }
    }

    return data;
}

void AnnualLogData::collect(const std::shared_ptr<LogRepositoryBase> &repo,
                            const std::chrono::year_month_day &date, bool skipFirstLine) {
    // remove all information in maps for this date first
    const auto monthDayDate = monthDay(date);

    for (auto &[_, tags] : tagsPerSection) {
        for (auto &[_, dates] : tags) {
            dates.erase(monthDayDate);
        }
        // remove all tags that after removal of this date would be empty (except for <any tag>)
        std::erase_if(tags, [monthDayDate](auto &tagAndDates) {
            const auto &[tag, dates] = tagAndDates;
            return tag != kAnyOrNoTag && dates.size() == 0;
        });
    }

    std::erase_if(tagsPerSection, [monthDayDate](auto &item) {
        const auto &[section, tags] = item;
        if (section == kAnySection) {
            return false;
        }
        assert(tags.contains(kAnyOrNoTag)); // <any tag> should always be present
        return section != kAnyOrNoTag && tags.size() == 1 && tags.at(kAnyOrNoTag).empty();
    });

    // collect as if it was empty
    collectEmpty(*this, repo, date, skipFirstLine);
}

} // namespace caps_log::log
