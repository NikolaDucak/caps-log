#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
const std::vector<std::string> valid  {
"* tag",
"* tag(info)",
"*   tag",
"*   tag(info)",
"*   tag:body",
"*   tag(info):body",
"*   tag   (info)   : body",

"* tag:body",
"* tag(info):body",
"* tag:(body)",
};

const std::vector<std::string> invalid  {
// invalid"
"* tag(:",
"* tag:)",
"*",
};

void parse(std::istream s) {
    
}

#include <sstream>

TEST(LogEntry, ParseTags) {
    //parse(std::istringstream{"std"});
}

TEST(LogEntry, ParseSections) {
}



