#include <gtest/gtest.h>

#include "log/annual_log_data.hpp"
#include "log/log_file.hpp"

#include "mocks.hpp"
#include "utils/date.hpp"

namespace caps_log::log::test {
using utils::date::monthDay;

const auto kTestContent1 = R"(
* tag 0 1
# section 1
* sametag 
* tag 1 1
* tag 1 2
# section 2
* sametag 
* tag 2 1
* tag 2 2

* tag 2 3
)";

const auto kTestContent2 = R"(
* sametag 
* tag diff 0 1
# section diff 1
* sametag 
* tag diff 1 1
* tag diff 1 2
* tag diff 1 3
# section 2
* sametag 
* tag 2 3
)";

TEST(YearOverviewDataTest, Collect) {
    const auto dummyDate1 = std::chrono::year_month_day{std::chrono::year{2020},
                                                        std::chrono::month{2}, std::chrono::day{3}};
    const auto dummyDate2 = std::chrono::year_month_day{std::chrono::year{2020},
                                                        std::chrono::month{2}, std::chrono::day{4}};
    auto dummyRepo = std::make_shared<DummyRepository>();
    dummyRepo->write({dummyDate1, kTestContent1});
    dummyRepo->write({dummyDate2, kTestContent2});
    auto data = AnnualLogData::collect(dummyRepo, dummyDate1.year());

    // inital collection
    EXPECT_EQ(data.datesWithLogs.size(), 2);
    EXPECT_EQ(data.datesWithLogs.contains(monthDay(dummyDate1)), true);
    EXPECT_EQ(data.datesWithLogs.contains(monthDay(dummyDate2)), true);

    // tags
    std::set<std::string> day1Tags = {"tag 0 1", "tag 1 1", "tag 1 2",
                                      "tag 2 1", "tag 2 2", "tag 2 3"};
    std::set<std::string> day2Tags = {"tag diff 0 1", "tag diff 1 1", "tag diff 1 2",
                                      "tag diff 1 3", "tag 2 3"};

    for (const auto &tag : day1Tags) {
        EXPECT_TRUE(data.datesWithTag[tag].contains(monthDay(dummyDate1)));
        if (day2Tags.contains(tag)) {
            EXPECT_TRUE(data.datesWithTag[tag].contains(monthDay(dummyDate2)));
        } else {
            EXPECT_FALSE(data.datesWithTag[tag].contains(monthDay(dummyDate2)));
        }
    }

    // sections
    std::set<std::string> day1Sections = {"section 1", "section 2"};
    std::set<std::string> day2Sections = {"section diff 1", "section 2"};

    for (const auto &section : day1Sections) {
        EXPECT_TRUE(data.datesWithSection[section].contains(monthDay(dummyDate1)));
        if (day2Sections.contains(section)) {
            EXPECT_TRUE(data.datesWithSection[section].contains(monthDay(dummyDate2)));
        } else {
            EXPECT_FALSE(data.datesWithSection[section].contains(monthDay(dummyDate2)));
        }
    }

    // dates per tag

    std::map<std::string, std::map<std::string, utils::date::Dates>> expectedTagsPerSection;
    expectedTagsPerSection["section 1"] = {
        {"sametag", {monthDay(dummyDate1), monthDay(dummyDate2)}},
        {"tag 1 1", {monthDay(dummyDate1)}},
        {"tag 1 2", {monthDay(dummyDate1)}},
        {"tag diff 1 1", {monthDay(dummyDate2)}},
        {"tag diff 1 2", {monthDay(dummyDate2)}},
        {"tag diff 1 3", {monthDay(dummyDate2)}}};

    expectedTagsPerSection["section 2"] = {
        {"sametag", {monthDay(dummyDate1), monthDay(dummyDate2)}},
        {"tag 2 1", {monthDay(dummyDate1)}},
        {"tag 2 2", {monthDay(dummyDate1)}},
        {"tag 2 3", {monthDay(dummyDate1), monthDay(dummyDate2)}}};

    expectedTagsPerSection[""] = {{"tag diff 0 1", {monthDay(dummyDate2)}},
                                  {"sametag", {monthDay(dummyDate2)}},
                                  {"tag 2 3", {monthDay(dummyDate2)}}};

    EXPECT_EQ(data.tagsPerSection.size(), expectedTagsPerSection.size());
    for (const auto &[section, tags] : expectedTagsPerSection) {
        EXPECT_EQ(data.tagsPerSection[section], tags);
    }

    // collection after removal of a log entry
    dummyRepo->remove(dummyDate1);
    data.collect(dummyRepo, dummyDate1);
    EXPECT_EQ(data.datesWithLogs.size(), 1);
    EXPECT_EQ(data.datesWithLogs.contains(monthDay(dummyDate1)), false);
    EXPECT_EQ(data.datesWithLogs.contains(monthDay(dummyDate2)), true);

    std::map<std::string, std::map<std::string, utils::date::Dates>> expectedTagsPerSection2;
    expectedTagsPerSection["section 1"] = {{"sametag", {monthDay(dummyDate2)}},
                                           {"tag diff 1 1", {monthDay(dummyDate2)}},
                                           {"tag diff 1 2", {monthDay(dummyDate2)}},
                                           {"tag diff 1 3", {monthDay(dummyDate2)}}};

    expectedTagsPerSection["section 2"] = {{"sametag", {monthDay(dummyDate2)}},
                                           {"tag 2 3", {monthDay(dummyDate2)}}};

    expectedTagsPerSection[""] = {{"tag diff 0 1", {monthDay(dummyDate2)}},
                                  {"sametag", {monthDay(dummyDate2)}},
                                  {"tag 2 3", {monthDay(dummyDate2)}}};

    EXPECT_EQ(data.tagsPerSection.size(), expectedTagsPerSection2.size());
    for (const auto &[section, tags] : expectedTagsPerSection2) {
        EXPECT_EQ(data.tagsPerSection[section], tags);
    }
}

} // namespace caps_log::log::test
