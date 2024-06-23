#include "annual_log_data.hpp"
#include "utils/date.hpp"

namespace caps_log::log {

using utils::date::monthDay;

namespace {
void collectEmpty(AnnualLogData &data, const std::shared_ptr<LogRepositoryBase> &repo,
                  std::chrono::year_month_day date, bool skipFirstLine) {
    const auto input = repo->read(date);

    // if there is no log to be processed, return
    if (not input) {
        data.datesWithLogs.erase(monthDay(date));
        return;
    }

    data.datesWithLogs.insert(monthDay(date));

    // then parse and set mentions again
    const auto parsedSections = input->readSectionTitles(skipFirstLine);
    const auto parsedTags = input->readTagTitles();

    for (const auto &tag : parsedTags) {
        data.datesWithTag[tag].insert(monthDay(date));
    }
    for (const auto &section : parsedSections) {
        data.datesWithSection[section].insert(monthDay(date));
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
            data.collect(repo, std::chrono::year_month_day{year, month, day}, skipFirstLine);
        }
    }

    return data;
}

void AnnualLogData::collect(const std::shared_ptr<LogRepositoryBase> &repo,
                            const std::chrono::year_month_day &date, bool skipFirstLine) {
    // remove all information in maps for this date first
    const auto monthDayDate = monthDay(date);
    std::erase_if(datesWithTag, [monthDayDate](auto &item) {
        auto &[_, dates] = item;
        dates.erase(monthDayDate);
        return dates.size() == 0;
    });

    std::erase_if(datesWithSection, [monthDayDate](auto &item) {
        auto &[_, dates] = item;
        dates.erase(monthDayDate);
        return dates.size() == 0;
    });

    // collect as if it was empty
    collectEmpty(*this, repo, date, skipFirstLine);
}

} // namespace caps_log::log
