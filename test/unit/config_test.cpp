#include "config.hpp"

#include <gmock/gmock-spec-builders.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/property_tree/ini_parser.hpp>

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
    EXPECT_EQ(config.getViewConfig().logView.annualCalendar.sundayStart,
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
    EXPECT_TRUE(config.getViewConfig().logView.annualCalendar.sundayStart);
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
    EXPECT_TRUE(config.getViewConfig().logView.annualCalendar.sundayStart);
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
    EXPECT_EQ(config.getViewConfig().logView.recentEventsWindow, 42);
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
