#include <fmt/format.h>
#include <gtest/gtest.h>

#include "log/log_file.hpp"
#include "utils/string.hpp"

namespace caps_log::log::testing {

namespace {
struct ParsingTestData {
    std::string text;
    std::set<std::string> expectedResult;
};

inline ParsingTestData makeTagParsingData(const std::string &base, std::string title) {
    return {fmt::format(fmt::runtime(base), title), {caps_log::utils::lowercase(title)}};
}

inline ParsingTestData makeSectionParsingData(const std::string &base, std::string title) {
    // prepend a newline since first line is ignored when it comes to parsing sections
    return {fmt::format(fmt::runtime("\n" + base), title), {caps_log::utils::lowercase(title)}};
}

const std::vector<ParsingTestData> validTagsTestData{
    makeTagParsingData("* {}", "tag"),
    makeTagParsingData("* {}", "TAG"),
    makeTagParsingData("*   {}", "TAG"),
    makeTagParsingData("*   {}\n", "TAG"),
    makeTagParsingData(" * {}    ", "TAG"),
    makeTagParsingData("    *   {}", "TAG"),

    makeTagParsingData("    *   {}", "Multi Word Title"),
    makeTagParsingData(" * {}", "Multi Word Title"),
    makeTagParsingData(
        "*   {}\n", "VERRY long tag title with a bunchhhh of words and words and words and words"),

    makeTagParsingData("*   {} (info)\n", "title 123"),
    makeTagParsingData("* {}: some other information", "tag title"),
    makeTagParsingData("* {} (minor info): some other information", "tag title"),
    makeTagParsingData("* {} (minor info): some other information\non two lines", "tag title"),
};

const std::vector<ParsingTestData> invalidTagsTestData{
    makeTagParsingData("*   {}     (123)\n", "123)"),
    makeTagParsingData("*   {}     ($$$$)    ", "123)"),
    makeTagParsingData("*   {}\n", "title with)"),
    makeTagParsingData("*   {}\n", "# title with"),
};

const std::vector<ParsingTestData> validSectionsTestData{
    makeSectionParsingData("# {}", "section"),
    makeSectionParsingData("# {}", "section title"),
    makeSectionParsingData("#     {}", "section title"),
    makeSectionParsingData("    # {}", "section"),
};

const std::vector<ParsingTestData> invalidSectionsTestData{
    makeSectionParsingData("## {}", "section"),
    makeSectionParsingData("### {}", "section"),
};

const std::chrono::year_month_day kDate{std::chrono::year{2021}, std::chrono::month{1},
                                        std::chrono::day{1}};

} // namespace

TEST(LogEntry, ParseTagTitles_Valid) {
    for (const auto &[text, expectedTagTitles] : validTagsTestData) {
        const auto parsedTagTitles = LogFile{kDate, text}.parse().getTagTitles();
        EXPECT_EQ(parsedTagTitles, expectedTagTitles) << "Failed at: " << text;
    }
}

TEST(LogEntry, ParseSectionTitles_Valid) {
    for (const auto &[text, expectedTagTitles] : validSectionsTestData) {
        const auto parsedSectionTitles = LogFile{kDate, text}.parse().getSectionTitles();
        EXPECT_EQ(parsedSectionTitles, expectedTagTitles) << "Failed at: " << text;
    }
}

TEST(LogEntry, ParseTagTitles_Invalid) {
    for (const auto &[text, _] : invalidSectionsTestData) {
        const auto parsedTagTitles = LogFile{kDate, text}.parse().getTagTitles();
        EXPECT_TRUE(parsedTagTitles.empty()) << "Failed at: " << text;
    }
}

TEST(LogEntry, ParseSectionTitles_Invalid) {
    for (const auto &[text, _] : invalidSectionsTestData) {
        const auto parsedSectionTitles = LogFile{kDate, text}.parse().getSectionTitles();
        EXPECT_TRUE(parsedSectionTitles.empty()) << "Failed at: " << text;
    }
}

TEST(LogEntry, ParseTagsPerSection) {
    const auto *content = R"(
* tag 0 1

# section 1
* tag 1 1
* tag 1 2
   
# section 2
* tag 2 1
* tag 2 2

* tag 2 3

# empty section
    )";

    auto parsedTagsPerSection = LogFile{kDate, content}.parse().getTagsPerSection();
    EXPECT_EQ(parsedTagsPerSection.size(), 4);
    const auto expectedSection1Tags = std::set<std::string>{"tag 1 1", "tag 1 2"};
    EXPECT_EQ(parsedTagsPerSection["section 1"], expectedSection1Tags);
    const auto expectedSection2Tags = std::set<std::string>{"tag 2 1", "tag 2 2", "tag 2 3"};
    EXPECT_EQ(parsedTagsPerSection["section 2"], expectedSection2Tags);
    const auto expectedSection0Tags = std::set<std::string>{"tag 0 1"};
    EXPECT_EQ(parsedTagsPerSection[LogFile::kRootSectionKey.data()], expectedSection0Tags);

    ASSERT_TRUE(parsedTagsPerSection.contains("empty section"));
    EXPECT_TRUE(parsedTagsPerSection.at("empty section").empty());
}

TEST(LogEntry, ParseSectionTitles_IgnoreSectionsInCodeBlocks) {
    const auto *sectionInCodeBlock = R"(
```
# section title
  # section title 2
```
    )";

    const auto *sectionInCodeBlock2 = R"(
```sh
# section title
  # section title 2
```
    )";

    auto parsedSectionTitles = LogFile{kDate, sectionInCodeBlock}.parse().getSectionTitles();
    EXPECT_TRUE(parsedSectionTitles.empty());
    parsedSectionTitles = LogFile{kDate, sectionInCodeBlock2}.parse().getSectionTitles();
    EXPECT_TRUE(parsedSectionTitles.empty());
}

TEST(LogEntry, ParseTagTitles_IgnoreTagInCodeBlocks) {
    const auto *const sectionInCodeBlock = R"(
```
* tag title
 * tag title 2
*   tag title 3
```
    )";
    const auto *const sectionInCodeBlock2 = R"(
```sh
* tag title
 * tag title 2
*   tag title 3
```
    )";

    auto parsedSectionTag = LogFile{kDate, sectionInCodeBlock}.parse().getTagTitles();
    EXPECT_TRUE(parsedSectionTag.empty());
    parsedSectionTag = LogFile{kDate, sectionInCodeBlock2}.parse().getTagTitles();
    EXPECT_TRUE(parsedSectionTag.empty());
}

} // namespace caps_log::log::testing
