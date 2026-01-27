#include "config.hpp"

#include <gmock/gmock-spec-builders.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/property_tree/ini_parser.hpp>
#include <ftxui/dom/elements.hpp>

using namespace caps_log;
using namespace testing;

std::function<std::string(const std::filesystem::path &)> makeMockReadFileFunc(
    const std::string &configContent,
    const std::filesystem::path &expectedPath = Configuration::kDefaultConfigLocation) {
    return [configContent, expectedPath](const std::filesystem::path &path) {
        if (path != expectedPath) {
            throw std::runtime_error("Unexpected config file path: " + path.string());
        }
        return configContent;
    };
}

TEST(ConfigTest, DefaultConfigurations) {
    std::vector<std::string> args = {"caps-log"};
    auto configFile = makeMockReadFileFunc("");

    Configuration config{args, configFile};

    EXPECT_EQ(config.getLogDirPath(), Configuration::kDefaultLogDirPath);
    EXPECT_EQ(config.getLogFilenameFormat(), Configuration::kDefaultLogFilenameFormat);
    EXPECT_EQ(config.getViewConfig().annualViewConfig.sundayStart,
              Configuration::kDefaultSundayStart);
    EXPECT_EQ(config.getAppConfig().skipFirstLine,
              !Configuration::kDefaultAcceptSectionsOnFirstLine);
    EXPECT_EQ(config.getPassword(), "");
    EXPECT_FALSE(config.getGitRepoConfig().has_value());
    EXPECT_TRUE(config.getAppConfig().events.empty());
}

TEST(ConfigTest, ConfigFileOverrides) {
    std::vector<std::string> args = {"caps-log"};

    std::string configContent = "log-dir-path=/override/path/\n"
                                "log-filename-format=override_format.md\n"
                                "sunday-start=true\n"
                                "first-line-section=false\n"
                                "password=override_password";
    auto configFile = makeMockReadFileFunc(configContent);

    Configuration config = Configuration(args, configFile);

    EXPECT_EQ(config.getLogDirPath(), "/override/path/");
    EXPECT_EQ(config.getLogFilenameFormat(), "override_format.md");
    EXPECT_TRUE(config.getViewConfig().annualViewConfig.sundayStart);
    EXPECT_TRUE(config.getAppConfig().skipFirstLine);
    EXPECT_EQ(config.getPassword(), "override_password");
    EXPECT_FALSE(config.getGitRepoConfig().has_value());
}

TEST(ConfigTest, CommandLineOverrides) {
    std::vector<std::string> args = {"caps-log",
                                     "--config",
                                     "some/path/to/config.ini",
                                     "--log-dir-path",
                                     "/cmd/override/path/",
                                     "--log-name-format",
                                     "cmd_override_format.md",
                                     "--sunday-start",
                                     "--first-line-section",
                                     "--password",
                                     "cmd_override_password"};

    std::string configContent = "log-dir-path=/file/override/path/\n"
                                "log-filename-format=file_override_format.md\n"
                                "sunday-start=false\n"
                                "first-line-section=true\n"
                                "password=file_override_password";
    auto configFile = makeMockReadFileFunc(configContent, "some/path/to/config.ini");

    Configuration config = Configuration(args, configFile);

    EXPECT_EQ(config.getLogDirPath(), "/cmd/override/path/");
    EXPECT_EQ(config.getLogFilenameFormat(), "cmd_override_format.md");
    EXPECT_TRUE(config.getViewConfig().annualViewConfig.sundayStart);
    EXPECT_FALSE(config.getAppConfig().skipFirstLine);
    EXPECT_EQ(config.getPassword(), "cmd_override_password");
    EXPECT_FALSE(config.getGitRepoConfig().has_value());
}

TEST(ConfigTest, GitConfigWorks) {
    std::string configContent = "log-dir-path=/path/to/repo/log-dir\n"
                                "[git]\n"
                                "enable-git-log-repo=true\n"
                                "repo-root=/path/to/repo/\n"
                                "ssh-key-path=/path/to/key\n"
                                "ssh-pub-key-path=/path/to/pub-key\n"
                                "main-branch-name=main-name\n"
                                "remote-name=remote-name";
    auto configfile = makeMockReadFileFunc(configContent);
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    Configuration config = Configuration(cmdLineArgs, configfile);

    EXPECT_TRUE(config.getGitRepoConfig().has_value());
    EXPECT_EQ(config.getGitRepoConfig()->root, "/path/to/repo/");
    EXPECT_EQ(config.getGitRepoConfig()->sshKeyPath, "/path/to/key");
    EXPECT_EQ(config.getGitRepoConfig()->sshPubKeyPath, "/path/to/pub-key");
    EXPECT_EQ(config.getGitRepoConfig()->mainBranchName, "main-name");
    EXPECT_EQ(config.getGitRepoConfig()->remoteName, "remote-name");
}

TEST(ConfigTest, GitConfigDisabledIfUnset) {
    std::string configContent = "log-dir-path=/path/to/repo/log-dir\n"
                                "[git]\n"
                                "repo-root=/path/to/repo/\n"
                                "ssh-key-path=/path/to/key\n"
                                "ssh-pub-key-path=/path/to/pub-key\n"
                                "main-branch-name=main-name\n"
                                "remote-name=remote-name";
    auto configFile = makeMockReadFileFunc(configContent);
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    Configuration config = Configuration(cmdLineArgs, configFile);

    EXPECT_FALSE(config.getGitRepoConfig().has_value());
}

TEST(ConfigTest, GitConfigDisabled) {
    std::string configContent = "log-dir-path=/path/to/repo/log-dir\n"
                                "[git]\n"
                                "enable-git-log-repo=false\n"
                                "repo-root=/path/to/repo/\n"
                                "ssh-key-path=/path/to/key\n"
                                "ssh-pub-key-path=/path/to/pub-key\n"
                                "main-branch-name=main-name\n"
                                "remote-name=remote-name";
    auto configFile = makeMockReadFileFunc(configContent);
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    Configuration config = Configuration(cmdLineArgs, configFile);

    EXPECT_FALSE(config.getGitRepoConfig().has_value());
}

TEST(ConfigTest, GitConfigThrowsIfLogDirIsNotInsideRepoRoot) {
    std::string configContent = "log-dir-path=/different/path/to/repo/log-dir\n"
                                "[git]\n"
                                "enable-git-log-repo=true\n"
                                "repo-root=/path/to/repo/\n"
                                "ssh-key-path=/path/to/key\n"
                                "ssh-pub-key-path=/path/to/pub-key\n"
                                "main-branch-name=main-name\n"
                                "remote-name=remote-name";
    auto configFile = makeMockReadFileFunc(configContent);
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    EXPECT_THROW(Configuration(cmdLineArgs, configFile).verify(), caps_log::ConfigParsingException);
}

TEST(ConfigTest, GitConfigDoesNotThrowIfGitRootIsSameAsLogDirPath) {
    std::string configContent = "log-dir-path=/path/to/repo/\n"
                                "[git]\n"
                                "enable-git-log-repo=true\n"
                                "repo-root=/path/to/repo/\n"
                                "ssh-key-path=/path/to/key\n"
                                "ssh-pub-key-path=/path/to/pub-key\n"
                                "main-branch-name=main-name\n"
                                "remote-name=remote-name";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_NO_THROW(Configuration(cmdLineArgs, configFile));
}

TEST(ConfigTest, GitConfigIsAppliedAfterCommandLineOverrides) {
    std::string configContent = "log-dir-path=/path/to/repo/\n"
                                "[git]\n"
                                "enable-git-log-repo=true\n"
                                "repo-root=/path/to/repo/\n"
                                "ssh-key-path=/path/to/key\n"
                                "ssh-pub-key-path=/path/to/pub-key\n"
                                "main-branch-name=main-name\n"
                                "remote-name=remote-name";
    std::vector<std::string> cmdLineArgs = {
        "caps-log",
        "--log-dir-path",
        "/cmd/override/path/",
    };
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile).verify(), caps_log::ConfigParsingException);
}

TEST(ConfigTest, CalendarEventsParsing) {
    std::string configContent = "log-dir-path=/path/to/repo/log-dir\n"
                                "[calendar-events]\n"
                                "recent-events-window=42\n"
                                "[calendar-events.birthdays.0]\n"
                                "name=John Doe\n"
                                "date=02.02.\n"
                                "[calendar-events.holidays.0]\n"
                                "name=Christmas\n"
                                "date=25.12.\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    Configuration config = Configuration(cmdLineArgs, configFile);

    auto events = config.getAppConfig().events;
    EXPECT_EQ(config.getViewConfig().annualViewConfig.recentEventsWindow, 42);
    EXPECT_EQ(events.size(), 2);
    EXPECT_EQ(events["birthdays"].size(), 1);
    EXPECT_EQ(events["holidays"].size(), 1);
    EXPECT_EQ(events["birthdays"].begin()->name, "John Doe");
    const auto date1 = std::chrono::month_day{std::chrono::month{2}, std::chrono::day{2}};
    EXPECT_EQ(events["birthdays"].begin()->date, date1);
    EXPECT_EQ(events["holidays"].begin()->name, "Christmas");
    const auto date2 = std::chrono::month_day{std::chrono::month{12}, std::chrono::day{25}};
    EXPECT_EQ(events["holidays"].begin()->date, date2);
}

TEST(ConfigTest, CalendarEventsParsing_ThrowsWhenNoId) {
    std::string configContent = "log-dir-path=/path/to/repo/log-dir\n"
                                "[calendar-events]\n"
                                "recent-events-window=42\n"
                                "[calendar-events.birthdays]\n"
                                "name=John Doe\n"
                                "date=02.02.\n"
                                "[calendar-events.holidays.0]\n"
                                "name=Christmas\n"
                                "date=12.25.\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile).verify(), caps_log::ConfigParsingException);
}

TEST(ConfigTest, CalendarEventsParsing_ThrowsWhenBadDate) {
    std::string configContent = "log-dir-path=/path/to/repo/log-dir\n"
                                "[calendar-events]\n"
                                "recent-events-window=42\n"
                                "[calendar-events.birthdays.0]\n"
                                "name=John Doe\n"
                                "date=02-02-02\n"
                                "[calendar-events.holidays.0]\n"
                                "name=Christmas\n"
                                "date=12.25.\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile).verify(), caps_log::ConfigParsingException);
}

TEST(ConfigTest, ThemeParsingAcceptsSupportedColorFormats) {
    std::string configContent = "[view.annual-view.theme.log-date]\n"
                                "fgcolor=rgb(12,34,56)\n"
                                "bgcolor=#aabbcc\n"
                                "[view.annual-view.theme.event-date]\n"
                                "fgcolor=ansi16(brightred)\n"
                                "bgcolor=ansi256(123)\n"
                                "bold=true\n"
                                "[view.annual-view.theme.todays-date]\n"
                                "fgcolor=rgba(1,2,3,255)\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_NO_THROW(Configuration(cmdLineArgs, configFile));
}

TEST(ConfigTest, ThemeParsingThrowsOnInvalidColor) {
    std::string configContent = "[view.annual-view.theme.event-date]\n"
                                "fgcolor=ansi256(999)\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile), caps_log::ConfigParsingException);
}

TEST(ConfigTest, ThemeParsingErrorIncludesKeyAndValue) {
    std::string configContent = "[view.annual-view.theme.event-date]\n"
                                "fgcolor=rgb(1,2)\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    try {
        Configuration config(cmdLineArgs, configFile);
        (void)config;
        FAIL() << "Expected ConfigParsingException";
    } catch (const caps_log::ConfigParsingException &e) {
        EXPECT_THAT(std::string(e.what()), HasSubstr("view.annual-view.theme.event-date.fgcolor"));
        EXPECT_THAT(std::string(e.what()), HasSubstr("rgb(1,2)"));
    }
}

TEST(ConfigTest, ThemeParsingThrowsOnInvalidHex) {
    std::string configContent = "[view.annual-view.theme.log-date]\n"
                                "fgcolor=#zzzzzz\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile), caps_log::ConfigParsingException);
}

TEST(ConfigTest, ThemeParsingThrowsOnInvalidAnsi16Name) {
    std::string configContent = "[view.annual-view.theme.log-date]\n"
                                "fgcolor=ansi16(not-a-color)\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile), caps_log::ConfigParsingException);
}

TEST(ConfigTest, ThemeParsingThrowsOnInvalidRgbaAlphaRange) {
    std::string configContent = "[view.annual-view.theme.log-date]\n"
                                "fgcolor=rgba(1,2,3,999)\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile), caps_log::ConfigParsingException);
}

TEST(ConfigTest, ThemeBorderParsingFromThemeSection) {
    std::string configContent = "[view.annual-view.theme]\n"
                                "calendar-border=double\n"
                                "calendar-month-border=light\n"
                                "tags-menu.border=heavy\n"
                                "sections-menu.border=dashed\n"
                                "events-list.border=empty\n"
                                "log-entry-preview.border=rounded\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    Configuration config(cmdLineArgs, configFile);

    const auto &theme = config.getViewConfig().annualViewConfig.theme;
    EXPECT_EQ(theme.calendarBorder, ftxui::BorderStyle::DOUBLE);
    EXPECT_EQ(theme.calendarMonthBorder, ftxui::BorderStyle::LIGHT);
    EXPECT_EQ(theme.tagsMenuConfig.border, ftxui::BorderStyle::HEAVY);
    EXPECT_EQ(theme.sectionsMenuConfig.border, ftxui::BorderStyle::DASHED);
    EXPECT_EQ(theme.eventsListConfig.border, ftxui::BorderStyle::EMPTY);
    EXPECT_EQ(theme.logEntryPreviewConfig.border, ftxui::BorderStyle::ROUNDED);
}

TEST(ConfigTest, ThemeBorderParsingFromBorderSubsections) {
    std::string configContent = "[view.annual-view.theme.calendar-border]\n"
                                "border=double\n"
                                "[view.annual-view.theme.calendar-month-border]\n"
                                "border=light\n"
                                "[view.annual-view.theme.tags-menu]\n"
                                "border=heavy\n"
                                "[view.annual-view.theme.sections-menu]\n"
                                "border=dashed\n"
                                "[view.annual-view.theme.events-list]\n"
                                "border=empty\n"
                                "[view.annual-view.theme.log-entry-preview]\n"
                                "border=rounded\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    Configuration config(cmdLineArgs, configFile);

    const auto &theme = config.getViewConfig().annualViewConfig.theme;
    EXPECT_EQ(theme.calendarBorder, ftxui::BorderStyle::DOUBLE);
    EXPECT_EQ(theme.calendarMonthBorder, ftxui::BorderStyle::LIGHT);
    EXPECT_EQ(theme.tagsMenuConfig.border, ftxui::BorderStyle::HEAVY);
    EXPECT_EQ(theme.sectionsMenuConfig.border, ftxui::BorderStyle::DASHED);
    EXPECT_EQ(theme.eventsListConfig.border, ftxui::BorderStyle::EMPTY);
    EXPECT_EQ(theme.logEntryPreviewConfig.border, ftxui::BorderStyle::ROUNDED);
}

TEST(ConfigTest, ThemeBorderParsingThrowsOnInvalidBorderValue) {
    std::string configContent = "[view.annual-view.theme]\n"
                                "calendar-border=not-a-border\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile), caps_log::ConfigParsingException);
}

TEST(ConfigTest, MarkdownThemeParsingForLogEntryPreview) {
    std::string configContent = "[view.annual-view.theme.log-entry-preview.markdown-theme]\n"
                                "header1=ansi256(220)\n"
                                "header2=ansi256(150)\n"
                                "header3=ansi256(115)\n"
                                "header4=ansi256(110)\n"
                                "header5=ansi256(117)\n"
                                "header6=ansi256(109)\n"
                                "list=ansi16(blue)\n"
                                "quote=ansi16(magenta)\n"
                                "code-fg=ansi16(brightblack)\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    Configuration config(cmdLineArgs, configFile);

    const auto &theme =
        config.getViewConfig().annualViewConfig.theme.logEntryPreviewConfig.markdownTheme;
    EXPECT_EQ(theme.headerColorForLevel(1), ftxui::Color::Palette256(220));
    EXPECT_EQ(theme.headerColorForLevel(2), ftxui::Color::Palette256(150));
    EXPECT_EQ(theme.headerColorForLevel(3), ftxui::Color::Palette256(115));
    EXPECT_EQ(theme.headerColorForLevel(4), ftxui::Color::Palette256(110));
    EXPECT_EQ(theme.headerColorForLevel(5), ftxui::Color::Palette256(117));
    EXPECT_EQ(theme.headerColorForLevel(6), ftxui::Color::Palette256(109));
    EXPECT_EQ(theme.list, ftxui::Color::Palette16::Blue);
    EXPECT_EQ(theme.quote, ftxui::Color::Palette16::Magenta);
    EXPECT_EQ(theme.codeFg, ftxui::Color::Palette16::GrayDark);
}

TEST(ConfigTest, MarkdownThemeParsingThrowsOnInvalidColor) {
    std::string configContent = "[view.annual-view.theme.log-entry-preview.markdown-theme]\n"
                                "header1=ansi256(999)\n";
    std::vector<std::string> cmdLineArgs = {"caps-log"};
    auto configFile = makeMockReadFileFunc(configContent);
    EXPECT_THROW(Configuration(cmdLineArgs, configFile), caps_log::ConfigParsingException);
}
