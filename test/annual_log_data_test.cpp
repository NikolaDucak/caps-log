#include <gtest/gtest.h>

#include "log/annual_log_data.hpp"
#include "log/log_file.hpp"

#include "mocks.hpp"
#include "utils/date.hpp"

namespace caps_log::log::test {

TEST(YearOverviewDataTest, Collect) {
    const auto dummyDate = std::chrono::year_month_day{std::chrono::year{2020},
                                                       std::chrono::month{2}, std::chrono::day{3}};
    auto dummyRepo = std::make_shared<DummyRepository>();
    dummyRepo->write({dummyDate, "\n# dummy section\n * dummy tag"});
    auto data = AnnualLogData::collect(dummyRepo, dummyDate.year());

    // inital collection
    EXPECT_EQ(data.datesWithTag.size(), 1);
    EXPECT_EQ(data.datesWithSection.size(), 1);
    EXPECT_EQ(data.datesWithLogs.size(), 1);
    EXPECT_EQ(data.datesWithLogs.contains(utils::date::monthDay(dummyDate)), true);

    EXPECT_EQ(data.datesWithTag["dummy tag"].size(), 1);
    EXPECT_EQ(data.datesWithSection["dummy section"].size(), 1);
    EXPECT_EQ(data.datesWithTag["dummy tag"].contains(utils::date::monthDay(dummyDate)), true);
    EXPECT_EQ(data.datesWithSection["dummy section"].contains(utils::date::monthDay(dummyDate)),
              true);

    // collection after removal of a log entry
    dummyRepo->remove(dummyDate);
    data.collect(dummyRepo, dummyDate);

    EXPECT_EQ(data.datesWithTag.size(), 0);
    EXPECT_EQ(data.datesWithSection.size(), 0);
    EXPECT_EQ(data.datesWithLogs.size(), 0);
}

} // namespace caps_log::log::test
