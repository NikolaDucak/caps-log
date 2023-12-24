#include "config.hpp"

#include <gmock/gmock-spec-builders.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace caps_log;
using namespace testing;
using MockFilesystemReader = testing::MockFunction<FileReader>;

// Utility function to convert a vector of strings to argv format and parse command line arguments
boost::program_options::variables_map parseArgs(const std::vector<std::string> &args) {
    std::vector<const char *> argv_temp;
    argv_temp.reserve(args.size());
    for (const auto &arg : args) {
        argv_temp.push_back(arg.data());
    }
    argv_temp.push_back(nullptr); // Null terminator for argv

    const char **argv = argv_temp.data();
    int argc = static_cast<int>(argv_temp.size()) - 1; // argc should not count the null terminator

    // Replace with your actual command line parsing logic
    return parseCLIOptions(argc, argv);
}

// Mock FileReader that returns a stringstream with preset configuration data
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

    EXPECT_EQ(config.logDirPath, Config::DEFAULT_LOG_DIR_PATH);
    EXPECT_EQ(config.logFilenameFormat, Config::DEFAULT_LOG_FILENAME_FORMAT);
    EXPECT_EQ(config.sundayStart, Config::DEFAULT_SUNDAY_START);
    EXPECT_EQ(config.ignoreFirstLineWhenParsingSections,
              Config::DEFAULT_IGNORE_FIRST_LINE_WHEN_PARSING_SECTIONS);
    EXPECT_EQ(config.password, "");
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

    std::string configContent = "[General]\n"
                                "log-dir-path=/file/override/path/\n"
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
}
