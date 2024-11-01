#include "annual_log_data.hpp"
#include "utils/date.hpp"
#include <iostream>

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

    std::cout << "COLLECT " << utils::date::formatToString(date) << std::endl;
    data.datesWithLogs.insert(monthDay(date));

    const auto monthDayDate = monthDay(date);
    for (const auto &[section, tags] : input->getTagsPerSection()) {
        std::cout << "COLLECT section: " << section << std::endl;
        data.tagsPerSection[section][AnnualLogData::kAnyTag].insert(monthDayDate);
        data.tagsPerSection[AnnualLogData::kAnySection][AnnualLogData::kAnyTag].insert(
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
    std::erase_if(tagsPerSection, [monthDayDate](auto &item) {
        auto &[section, tags] = item;
        const auto numOfErased = std::erase_if(tags, [monthDayDate, section](auto &tagAndDates) {
            auto &[tag, dates] = tagAndDates;
            dates.erase(monthDayDate);
            if (tag == kAnyTag) {
                return false;
            }
            return dates.size() == 0;
        });
        if (section == kAnySection) {
            return false;
        }
        return tags.size() <= 1; // 1 because of the kAnyTag
    });

    // collect as if it was empty
    collectEmpty(*this, repo, date, skipFirstLine);
}

} // namespace caps_log::log
