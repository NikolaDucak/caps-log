#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fmt/format.h>
#include <sstream>


/*
"* tag",
"* Tag",
"* TAG",
"* tag title",
"* Tag Title",
"* TagTitle",
"* verry long tag title with a bunch of words yada yadda",
"* Verry Log tag tiTLe Bla blas",
"* tag title       (info)",
"* Tag stuff",
"*    tag",
"* tag(info)",
"* tag: body",
"*    tag(info);",
" * section",
"    * sections",
*/

struct TagTestData {
    std::string base;
    std::string title;
    std::string info;
    std::string toString() {
        return fmt::format(base, title);
    }
};

const std::string TAG_REGEX { R"(^( +)?\*( +)([a-z A-Z]+)(\(.+\))?(:?))" };

const std::vector<std::string> invalidTags  {
// invalid"
"* tag(:",
"* tag:)",
"*",
"  some other words * realy long tag title with a lot of words(and info): and a body",
};

const std::vector<std::string> validSections { };
const std::vector<std::string> invalidSections { };

std::string parse(std::istream s) { }


TEST(LogEntry, ParseTags) {
    for (line: testCases) {
        auto c = parse(line);
    }
}

TEST(LogEntry, ParseSections) {
}



