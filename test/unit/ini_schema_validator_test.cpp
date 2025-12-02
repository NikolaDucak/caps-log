// test_ini_schema_validation.cpp
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "config/ini_schema.hpp"

namespace {

using boost::property_tree::ptree;

ptree makeArray(const std::vector<ptree> &items) {
    ptree arr;
    for (const auto &it : items) {
        arr.push_back(std::make_pair(std::string{}, it)); // property_tree "array"
    }
    return arr;
}

} // namespace

namespace caps_log::config::test {

class IniSchemaValidationTest : public ::testing::Test {
  protected:
    IniSchema schema = getCapsLogIniSchema();
    std::vector<std::string> errors;

    bool run(const ptree &cfg) {
        errors.clear();
        return schema.validate(cfg, errors);
    }

    [[nodiscard]] bool hasErrorContaining(const std::string &needle) const {
        for (const auto &err : errors) {
            if (err.find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};

TEST_F(IniSchemaValidationTest, EmptyConfig_Passes) {
    ptree cfg; // all sections optional unless present
    EXPECT_TRUE(run(cfg));
    EXPECT_TRUE(errors.empty());
}

TEST_F(IniSchemaValidationTest, GitSection_MissingRequiredProps_Fails) {
    ptree cfg;
    // Present the "git" section but omit most required fields
    cfg.put("git.enable-git-log-repo", "true");

    EXPECT_FALSE(run(cfg));
    // A few representative required fields should be reported as missing:
    EXPECT_TRUE(hasErrorContaining("[git.ssh-pub-key-path] required property missing"));
    EXPECT_TRUE(hasErrorContaining("[git.ssh-key-path] required property missing"));
    EXPECT_TRUE(hasErrorContaining("[git.main-branch-name] required property missing"));
    EXPECT_TRUE(hasErrorContaining("[git.remote-name] required property missing"));
    EXPECT_TRUE(hasErrorContaining("[git.repo-root] required property missing"));
}

TEST_F(IniSchemaValidationTest, GitSection_EmptyStringWhereRequired_Fails) {
    ptree cfg;
    cfg.put("git.enable-git-log-repo", "true");
    cfg.put("git.ssh-pub-key-path", "/home/user/.ssh/id_rsa.pub");
    cfg.put("git.ssh-key-path", "/home/user/.ssh/id_rsa");
    cfg.put("git.main-branch-name", ""); // <- empty, should fail
    cfg.put("git.remote-name", "origin");
    cfg.put("git.repo-root", "/repos/caps-log");

    EXPECT_FALSE(run(cfg));
    EXPECT_TRUE(hasErrorContaining("[git.main-branch-name] non-empty string/path expected"));
}

TEST_F(IniSchemaValidationTest, CalendarEvents_ValidArray_Passes) {
    ptree cfg;

    // calendar-events.birthdays = [ {name: "...", date: "06-01"} ]
    ptree item;
    item.put("name", "Mom's Birthday");
    item.put("date", "06-01");

    ptree calEvents; // children of calendar-events
    calEvents.add_child("birthdays", makeArray({item}));

    cfg.add_child("calendar-events", calEvents);

    EXPECT_TRUE(run(cfg)) << (errors.empty() ? "" : errors.front());
    EXPECT_TRUE(errors.empty());
}

TEST_F(IniSchemaValidationTest, CalendarEvents_InvalidDate_Fails) {
    ptree cfg;

    ptree bad;
    bad.put("name", "Weird Day");
    bad.put("date", "13-40"); // invalid mm-dd

    ptree calEvents;
    calEvents.add_child("birthdays", makeArray({bad}));
    cfg.add_child("calendar-events", calEvents);

    EXPECT_FALSE(run(cfg));
    EXPECT_TRUE(hasErrorContaining("[calendar-events.birthdays[0].date] date expected in MM-DD"));
}

TEST_F(IniSchemaValidationTest, ColorVariants_Valid_Passes) {
    ptree cfg;
    // hex(#abc), rgb(...), 256(...)
    cfg.put("ui.logs-view.tags-menu.color", "hex(#abc)");
    cfg.put("ui.logs-view.tags-menu.selected-color", "rgb(12,34,56)");
    cfg.put("ui.logs-view.annual-calendar.event-day-color", "256(42)");

    EXPECT_TRUE(run(cfg)) << (errors.empty() ? "" : errors.front());
    EXPECT_TRUE(errors.empty());
}

TEST_F(IniSchemaValidationTest, Color_Invalid_Fails) {
    ptree cfg;
    cfg.put("ui.logs-view.tags-menu.color", "rgb(300,0,0)"); // 300 out of range

    EXPECT_FALSE(run(cfg));
    EXPECT_TRUE(hasErrorContaining("[ui.logs-view.tags-menu.color] color expected"));
}

TEST_F(IniSchemaValidationTest, Style_ValidCombos_Passes) {
    ptree cfg;
    cfg.put("ui.logs-view.tags-menu.style", "bold, underline");
    cfg.put("ui.logs-view.annual-calendar.weekend-style", "italic");

    EXPECT_TRUE(run(cfg)) << (errors.empty() ? "" : errors.front());
    EXPECT_TRUE(errors.empty());
}

TEST_F(IniSchemaValidationTest, Style_InvalidToken_Fails) {
    ptree cfg;
    cfg.put("ui.logs-view.tags-menu.style", "blink"); // not allowed

    EXPECT_FALSE(run(cfg));
    EXPECT_TRUE(hasErrorContaining("[ui.logs-view.tags-menu.style] style expected"));
}

TEST_F(IniSchemaValidationTest, BooleanAndInteger_Parsing) {
    ptree cfg;
    // Booleans allow true/false/yes/no/on/off/1/0
    cfg.put("sunday-start", "yes");
    cfg.put("first-line-section", "off");

    // Integer is optional; but if present and invalid, it should fail
    cfg.put("recent-events-window", "ten"); // invalid integer

    EXPECT_FALSE(run(cfg));
    EXPECT_TRUE(hasErrorContaining("[recent-events-window] integer (base-10) expected"));
}

} // namespace caps_log::config::test
