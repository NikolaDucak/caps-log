#include <gtest/gtest.h>
#include <fmt/format.h>

#include "model/log_file.hpp"
#include "model/year_overview_data.hpp"

#include "mocks.hpp"

TEST(YearOverviewDataTest, Collect) {
    auto dummyDate = clog::date::Date{10,10,2020};
    auto dummyRepo = std::make_shared<DummyRepository>();
    dummyRepo->write({dummyDate, "\n# dummy section\n * dummy tag"});
    auto data = clog::model::YearOverviewData::collect(dummyRepo, dummyDate.year);

    // inital collection
    ASSERT_EQ(data.tagMap.size(), 1);
    ASSERT_EQ(data.sectionMap.size(), 1);
    ASSERT_EQ(data.logAvailabilityMap.daysSet(), 1);

    ASSERT_EQ(data.tagMap["dummy tag"].daysSet(), 1);
    ASSERT_EQ(data.sectionMap["dummy section"].daysSet(), 1);
    ASSERT_EQ(data.logAvailabilityMap.get(dummyDate), true);
    ASSERT_EQ(data.tagMap["dummy tag"].get(dummyDate), true);
    ASSERT_EQ(data.sectionMap["dummy section"].get(dummyDate), true);

    // collection after removal of a log entry
    dummyRepo->remove(dummyDate);
    data.collect(dummyRepo, dummyDate);
    
    ASSERT_EQ(data.tagMap.size(), 0);
    ASSERT_EQ(data.sectionMap.size(), 0);
    ASSERT_EQ(data.logAvailabilityMap.daysSet(), 0);
}
