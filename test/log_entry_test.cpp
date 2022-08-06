#include <fmt/format.h>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

#include "fmt/core.h"
#include "model/log_file.hpp"
#include "model/year_overview_data.hpp"
#include "utils/string.hpp"

#include "mocks.hpp"

struct ParsingTestData {
    std::string text;
    std::vector<std::string> expectedResult;
};

inline ParsingTestData makeTagParsingData(std::string base, std::string title) {
    return {fmt::format(base, title), {clog::utils::lowercase(title)}};
}

inline ParsingTestData makeSectionParsingData(std::string base, std::string title) {
    // prepend a newline since first line is ignored when it comes to parsing sections
    return {fmt::format(std::string{"\n"} + base, title), {clog::utils::lowercase(title)}};
}

namespace {
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

} // namespace

TEST(LogEntry, ParseTagTitles_Valid) {
    for (auto [text, expectedTagTitles] : validTagsTestData) {
        auto ss = std::stringstream{text};
        auto parsedTagTitles = clog::model::LogFile::readTagTitles(ss);
        EXPECT_EQ(parsedTagTitles, expectedTagTitles) << "Failed at: " << text;
    }
}

TEST(LogEntry, ParseSectionTitles_Valid) {
    for (auto [text, expectedTagTitles] : validSectionsTestData) {
        auto ss = std::stringstream{text};
        auto parsedTagTitles = clog::model::LogFile::readSectionTitles(ss);
        EXPECT_EQ(parsedTagTitles, expectedTagTitles) << "Failed at: " << text;
    }
}

TEST(LogEntry, ParseTagTitles_Invalid) {
    for (auto [text, _] : invalidSectionsTestData) {
        auto ss = std::stringstream{text};
        auto parsedTagTitles = clog::model::LogFile::readTagTitles(ss);
        EXPECT_TRUE(parsedTagTitles.empty()) << "Failed at: " << text;
    }
}

TEST(LogEntry, ParseSectionTitles_Invalid) {
    for (auto [text, _] : invalidSectionsTestData) {
        auto ss = std::stringstream{text};
        auto parsedTagTitles = clog::model::LogFile::readSectionTitles(ss);
        EXPECT_TRUE(parsedTagTitles.empty()) << "Failed at: " << text;
    }
}
