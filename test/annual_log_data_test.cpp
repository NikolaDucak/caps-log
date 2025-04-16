#include <gtest/gtest.h>

#include "log/annual_log_data.hpp"
#include "log/log_file.hpp"

#include "mocks.hpp"
#include "utils/date.hpp"

namespace caps_log::log::test {

namespace {
using utils::date::monthDay;
const auto &kAnySection = AnnualLogData::kAnySection;
const auto &kAnyTag = AnnualLogData::kAnyOrNoTag;
const auto dummyDate1 = std::chrono::year_month_day{std::chrono::year{2020}, std::chrono::month{2},
                                                    std::chrono::day{3}};
const auto dummyDate2 = std::chrono::year_month_day{std::chrono::year{2020}, std::chrono::month{2},
                                                    std::chrono::day{4}};
const auto dummyDate3 = std::chrono::year_month_day{std::chrono::year{2020}, std::chrono::month{2},
                                                    std::chrono::day{5}};

const auto md1 = monthDay(dummyDate1);
const auto md2 = monthDay(dummyDate2);
const auto md3 = monthDay(dummyDate3);
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
const auto kTestContent3 = R"(
some log content that doesnt have a tag or a section
)";
} // namespace

TEST(YearOverviewDataTest, Collect) {

    auto dummyRepo = std::make_shared<DummyRepository>();

    dummyRepo->write({dummyDate1, kTestContent1});
    dummyRepo->write({dummyDate2, kTestContent2});
    dummyRepo->write({dummyDate3, kTestContent3});

    auto data = AnnualLogData::collect(dummyRepo, dummyDate1.year());

    // initial collection
    const std::set<std::chrono::month_day> allLogs{md1, md2, md3};
    EXPECT_EQ(data.datesWithLogs, allLogs);

    {
        SCOPED_TRACE("Initial collection");
        // clang-format off
        std::map<std::string, std::map<std::string, utils::date::Dates>> expectedTagsPerSection;
        expectedTagsPerSection["section 1"] = {
            {kAnyTag,   {md1}},
            {"sametag", {md1}},
            {"tag 1 1", {md1}},
            {"tag 1 2", {md1}},
        };

        expectedTagsPerSection["section diff 1"] = {
            {kAnyTag,        {md2}},        
            {"sametag",      {md2}},
            {"tag diff 1 1", {md2}},
            {"tag diff 1 2", {md2}}, 
            {"tag diff 1 3", {md2}},
        };

        expectedTagsPerSection["section 2"] = {
            {kAnyTag,   {md1, md2}},
            {"sametag", {md1, md2}},
            {"tag 2 1", {md1}},
            {"tag 2 2", {md1}},
            {"tag 2 3", {md1, md2}},
        };

        expectedTagsPerSection[kAnySection] = {
            {kAnyTag,        {md1, md2}},
            {"sametag",      {md1, md2}},
            {"tag 0 1",      {md1}},
            {"tag 1 1",      {md1}},      
            {"tag 1 2",      {md1}},
            {"tag 2 1",      {md1}},
            {"tag 2 2",      {md1}},
            {"tag 2 3",      {md1 , md2}},
            {"tag diff 0 1", {md2}},
            {"tag diff 1 1", {md2}},
            {"tag diff 1 2", {md2}},
            {"tag diff 1 3", {md2}},
        };

        expectedTagsPerSection[LogFile::kRootSectionKey.data()] = {
            {kAnyTag,        {md1, md2}},
            {"tag 0 1",      {md1}},
            {"sametag",      {md2}},
            {"tag diff 0 1", {md2}},
        };
        // clang-format on

        EXPECT_EQ(data.tagsPerSection.size(), expectedTagsPerSection.size());
        for (const auto &[section, tags] : expectedTagsPerSection) {
            EXPECT_EQ(data.tagsPerSection[section], tags) << "Failed at: " << section;
        }
    }

    {
        SCOPED_TRACE("Collection after removal of a log entry");
        dummyRepo->remove(dummyDate1);
        data.collect(dummyRepo, dummyDate1);
        const std::set<std::chrono::month_day> allLogs{md2, md3};
        EXPECT_EQ(data.datesWithLogs, allLogs);

        // clang-format off
        std::map<std::string, std::map<std::string, utils::date::Dates>> postRemovalTagsPerSection;
        postRemovalTagsPerSection["section diff 1"] = {
            {kAnyTag,        {md2}},
            {"sametag",      {md2}},
            {"tag diff 1 1", {md2}},
            {"tag diff 1 2", {md2}},
            {"tag diff 1 3", {md2}},
        };

        postRemovalTagsPerSection["section 2"] = {
            {kAnyTag,   {md2}},
            {"sametag", {md2}},
            {"tag 2 3", {md2}},
        };

        postRemovalTagsPerSection[LogFile::kRootSectionKey.data()] = {
            {kAnyTag,        {md2}},
            {"tag diff 0 1", {md2}},
            {"sametag",      {md2}},
        };

        postRemovalTagsPerSection[kAnySection] = {
            {kAnyTag,        {md2}},        
            {"sametag",      {md2}},      
            {"tag 2 3",      {md2}},
            {"tag diff 0 1", {md2}}, 
            {"tag diff 1 1", {md2}}, 
            {"tag diff 1 2", {md2}},
            {"tag diff 1 3", {md2}},
        };
        //clang-format on

        EXPECT_EQ(data.tagsPerSection.size(), postRemovalTagsPerSection.size());
        for (const auto &[section, tags] : postRemovalTagsPerSection) {
            EXPECT_EQ(data.tagsPerSection.at(section), tags) << "Failed at: " << section;
        }
    }
}

TEST(YearOverviewDataTest, SectionWithNoTagPresent) {
    auto dummyRepo = std::make_shared<DummyRepository>();
    dummyRepo->write(LogFile{dummyDate1, "# DummyContent \n# section 1 \n* tag"});
    dummyRepo->write(LogFile{dummyDate2, "# DummyContent \n# section 1 \nno tag"});
    dummyRepo->write(LogFile{dummyDate3, "# DummyContent \n# section 2 \n* tag"});

    auto data = AnnualLogData::collect(dummyRepo, dummyDate1.year());

    std::map<std::string, std::map<std::string, utils::date::Dates>> expectedTagsPerSection;
    // clang-format off
    expectedTagsPerSection["section 1"] = {
        {kAnyTag,   {md1, md2}},
        {"tag",     {md1}},
    };
    expectedTagsPerSection["section 2"] = {
        {kAnyTag,   {md3}},
        {"tag",     {md3}},
    };
    expectedTagsPerSection[kAnySection] = {
        {kAnyTag,   {md1, md2, md3}},
        {"tag",     {md1, md3}},
    };

    // clang-format on
    for (const auto &[section, tags] : expectedTagsPerSection) {
        EXPECT_EQ(data.tagsPerSection[section], tags) << "Failed at: " << section;
    }
}

} // namespace caps_log::log::test
