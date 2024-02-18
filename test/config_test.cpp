#include "config.hpp"

#include <gmock/gmock-spec-builders.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace caps_log;
using namespace testing;
using MockFilesystemReader = testing::MockFunction<FileReader>;

boost::program_options::variables_map parseArgs(const std::vector<std::string> &args) {
    std::vector<const char *> argvTemp;
    argvTemp.reserve(args.size());
    for (const auto &arg : args) {
        argvTemp.push_back(arg.data());
    }
    argvTemp.push_back(nullptr); // Null terminator for argv

    const char **argv = argvTemp.data();
    int argc = static_cast<int>(argvTemp.size()) - 1; // argc should not count the null terminator

    return parseCLIOptions(argc, argv);
}

std::function<std::unique_ptr<std::istringstream>(const std::string &)>
mockFileReader(const std::string &configContent) {
    return [configContent](const std::string &) -> std::unique_ptr<std::istringstream> {
        return std::make_unique<std::istringstream>(std::istringstream(configContent));
    };
}

TEST(ConfigTest, DefaultConfigurations) {
    std::vector<std::string> args = {"caps-log"};
    auto cmdLineArgs = parseArgs(args);

    auto fileReader = mockFileReader("");

    Config config = Config::make(fileReader, cmdLineArgs);

    EXPECT_EQ(config.logDirPath, Config::kDefaultLogDirPath);
    EXPECT_EQ(config.logFilenameFormat, Config::kDefaultLogFilenameFormat);
    EXPECT_EQ(config.sundayStart, Config::kDefaultSundayStart);
    EXPECT_EQ(config.ignoreFirstLineWhenParsingSections,
              Config::kDefaultIgnoreFirstLineWhenParsingSections);
    EXPECT_EQ(config.password, "");
    EXPECT_FALSE(config.repoConfig.has_value());
}

TEST(ConfigTest, ConfigFileOverrides) {
    std::vector<std::string> args = {"caps-log"};
    auto cmdLineArgs = parseArgs(args);

    std::string configContent = "log-dir-path=/override/path/\n"
                                "log-filename-format=override_format.md\n"
                                "sunday-start=true\n"
                                "first-line-section=false\n"
                                "password=override_password";
    auto fileReader = mockFileReader(configContent);

    Config config = Config::make(fileReader, cmdLineArgs);

    EXPECT_EQ(config.logDirPath, "/override/path/");
    EXPECT_EQ(config.logFilenameFormat, "override_format.md");
    EXPECT_TRUE(config.sundayStart);
    EXPECT_FALSE(config.ignoreFirstLineWhenParsingSections);
    EXPECT_EQ(config.password, "override_password");
    EXPECT_FALSE(config.repoConfig.has_value());
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
    auto cmdLineArgs = parseArgs(args);

    std::string configContent = "log-dir-path=/file/override/path/\n"
                                "log-filename-format=file_override_format.md\n"
                                "sunday-start=false\n"
                                "first-line-section=true\n"
                                "password=file_override_password";
    auto fileReader = mockFileReader(configContent);

    Config config = Config::make(fileReader, cmdLineArgs);

    EXPECT_EQ(config.logDirPath, "/cmd/override/path/");
    EXPECT_EQ(config.logFilenameFormat, "cmd_override_format.md");
    EXPECT_TRUE(config.sundayStart);
    EXPECT_TRUE(config.ignoreFirstLineWhenParsingSections);
    EXPECT_EQ(config.password, "cmd_override_password");
    EXPECT_FALSE(config.repoConfig.has_value());
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
    auto cmdLineArgs = parseArgs({"caps-log"});
    auto fileReader = mockFileReader(configContent);
    Config config = Config::make(fileReader, cmdLineArgs);

    ASSERT_TRUE(config.repoConfig.has_value());
    EXPECT_EQ(config.repoConfig->root, "/path/to/repo/");
    EXPECT_EQ(config.repoConfig->sshKeyPath, "/path/to/key");
    EXPECT_EQ(config.repoConfig->sshPubKeyPath, "/path/to/pub-key");
    EXPECT_EQ(config.repoConfig->mainBranchName, "main-name");
    EXPECT_EQ(config.repoConfig->remoteName, "remote-name");
}

TEST(ConfigTest, GitConfigDisabledIfUnset) {
    std::string configContent = "log-dir-path=/path/to/repo/log-dir\n"
                                "[git]\n"
                                "repo-root=/path/to/repo/\n"
                                "ssh-key-path=/path/to/key\n"
                                "ssh-pub-key-path=/path/to/pub-key\n"
                                "main-branch-name=main-name\n"
                                "remote-name=remote-name";
    auto cmdLineArgs = parseArgs({"caps-log"});
    auto fileReader = mockFileReader(configContent);
    Config config = Config::make(fileReader, cmdLineArgs);

    ASSERT_FALSE(config.repoConfig.has_value());
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
    auto cmdLineArgs = parseArgs({"caps-log"});
    auto fileReader = mockFileReader(configContent);
    Config config = Config::make(fileReader, cmdLineArgs);

    ASSERT_FALSE(config.repoConfig.has_value());
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
    auto cmdLineArgs = parseArgs({"caps-log"});
    auto fileReader = mockFileReader(configContent);
    ASSERT_THROW(Config::make(fileReader, cmdLineArgs), std::invalid_argument);
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
    auto cmdLineArgs = parseArgs({"caps-log"});
    auto fileReader = mockFileReader(configContent);
    ASSERT_NO_THROW(Config::make(fileReader, cmdLineArgs));
}
