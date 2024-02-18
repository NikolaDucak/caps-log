#include <gtest/gtest.h>

#include "log/annual_log_data.hpp"
#include "log/log_file.hpp"

#include "mocks.hpp"

TEST(YearOverviewDataTest, Collect) {
    const auto dummyDate = std::chrono::year_month_day{std::chrono::year{2020},
                                                       std::chrono::month{2}, std::chrono::day{3}};
    auto dummyRepo = std::make_shared<DummyRepository>();
    dummyRepo->write({dummyDate, "\n# dummy section\n * dummy tag"});
    auto data = caps_log::log::AnnualLogData::collect(dummyRepo, dummyDate.year());

    // inital collection
    EXPECT_EQ(data.tagMap.size(), 1);
    EXPECT_EQ(data.sectionMap.size(), 1);
    EXPECT_EQ(data.logAvailabilityMap.daysSet(), 1);
    EXPECT_EQ(data.logAvailabilityMap.get(dummyDate), true);

    EXPECT_EQ(data.tagMap["dummy tag"].daysSet(), 1);
    EXPECT_EQ(data.sectionMap["dummy section"].daysSet(), 1);
    EXPECT_EQ(data.tagMap["dummy tag"].get(dummyDate), true);
    EXPECT_EQ(data.sectionMap["dummy section"].get(dummyDate), true);

    // collection after removal of a log entry
    dummyRepo->remove(dummyDate);
    data.collect(dummyRepo, dummyDate);

    EXPECT_EQ(data.tagMap.size(), 0);
    EXPECT_EQ(data.sectionMap.size(), 0);
    EXPECT_EQ(data.logAvailabilityMap.daysSet(), 0);
}
