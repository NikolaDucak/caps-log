#include "arg_parser.hpp"
#include "config.hpp"

#include <gmock/gmock-spec-builders.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace clog;
using namespace testing;
using MockFilesystemReader = testing::MockFunction<FileReader>;

TEST(ConfigTest, ParseCmdLineArgs_LogDirPath_NoPathProvided) {
    auto mockFS = MockFilesystemReader{};
    const char *argv[] = {"--log-dir-path"};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION)).WillOnce(Return(ByMove(nullptr)));
    ASSERT_ANY_THROW(Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv}));
}

TEST(ConfigTest, ParseCmdLineArgs_LogDirPath_OK) {
    auto mockFS = MockFilesystemReader{};
    const auto dummyLogDirPath = "/path/to/log/dir";
    const char *argv[] = {"--log-dir-path", dummyLogDirPath};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION)).WillOnce(Return(ByMove(nullptr)));
    const auto config =
        Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv});
    EXPECT_EQ(config.logDirPath, dummyLogDirPath);
}

TEST(ConfigTest, ParseCmdLineArgs_LogFilenameFormat_NoFormatProvided) {
    auto mockFS = MockFilesystemReader{};
    const auto dummyLogFilenameFormat = "%d_%m_%Y.txt";
    const char *argv[] = {"--log-filename-format", dummyLogFilenameFormat};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION)).WillOnce(Return(ByMove(nullptr)));
    const auto config =
        Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv});
    EXPECT_EQ(config.logFilenameFormat, dummyLogFilenameFormat);
}

TEST(ConfigTest, ParseCmdLineArgs_SundayStart) {
    auto mockFS = MockFilesystemReader{};
    const char *argv[] = {"--sunday-start", "some extra ignored param"};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION)).WillOnce(Return(ByMove(nullptr)));
    const auto config =
        Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv});
    EXPECT_EQ(config.sundayStart, true);
}

TEST(ConfigTest, ParseCmdLineArgs_PasswordWithNoValue) {
    auto mockFS = MockFilesystemReader{};
    const char *argv[] = {"--password"};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION)).WillOnce(Return(ByMove(nullptr)));
    ASSERT_ANY_THROW(Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv}));
}

TEST(ConfigTest, ParseCmdLineArgs_PasswordProvided) {
    auto mockFS = MockFilesystemReader{};
    const std::string password = {"dummy"};
    const char *argv[] = {"--password", password.c_str()};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION)).WillOnce(Return(ByMove(nullptr)));
    auto config = Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv});
    EXPECT_EQ(config.password, password);
}

TEST(ConfigTest, ParseConfigFile_LogDirPath_NoPathProvided) {
    auto mockFS = MockFilesystemReader{};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION))
        .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("log-dir-path = "))));

    auto config = Config::make(mockFS.AsStdFunction(), {});
    ASSERT_EQ(config.logDirPath, Config::DEFAULT_LOG_DIR_PATH);
}

TEST(ConfigTest, ParseConfigFile_LogDirPath_OK) {
    auto mockFS = MockFilesystemReader{};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION))
        .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("log-dir-path = /ok/path"))));

    auto config = Config::make(mockFS.AsStdFunction(), {});
    ASSERT_EQ(config.logDirPath, "/ok/path");
}

TEST(ConfigTest, ParseConfigFile_SundayStart_OK) {
    auto mockFS = MockFilesystemReader{};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION))
        .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("sunday-start = true"))));

    auto config = Config::make(mockFS.AsStdFunction(), {});
    ASSERT_EQ(config.sundayStart, true);
}

TEST(ConfigTest, ParseConfigFile_SundayStart_BadParam) {
    auto mockFS = MockFilesystemReader{};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION))
        .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("sunday-start = 123"))));

    auto config = Config::make(mockFS.AsStdFunction(), {});
    ASSERT_EQ(config.sundayStart, false);
}

TEST(ConfigTest, ParseConfigFile_Password_OK) {
    auto mockFS = MockFilesystemReader{};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION))
        .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("password = 123"))));

    auto config = Config::make(mockFS.AsStdFunction(), {});
    ASSERT_EQ(config.password, std::string{"123"});
}

TEST(ConfigTest, CMDLineArg_OverridesConfigFileArg) {
    auto mockFS = MockFilesystemReader{};
    const auto dummyLogDirPath = "/path/to/log/dir";
    const char *argv[] = {"--log-dir-path", dummyLogDirPath};

    EXPECT_CALL(mockFS, Call(Config::DEFAULT_CONFIG_LOCATION))
        .WillOnce(Return(ByMove(std::make_unique<std::istringstream>(
            "log-dir-path = /path/that/should/be/overridden"))));

    auto config2 = Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv});
    EXPECT_EQ(config2.logDirPath, dummyLogDirPath);
}

TEST(ConfigTest, CMDLineConfigArg_OverridesConfigFile) {
    auto mockFS = MockFilesystemReader{};
    const auto overrideConfigPath = "/path/to/override/config.ini";
    const char *argv[] = {"-c", overrideConfigPath};

    EXPECT_CALL(mockFS, Call(overrideConfigPath))
        .WillOnce(Return(ByMove(std::make_unique<std::istringstream>("log-dir-path = /new/path"))));

    auto config2 = Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv});
    EXPECT_EQ(config2.logDirPath, "/new/path");
}

TEST(ConfigTest, CMDLineArg_OverridesOverridenConfigFile) {
    auto mockFS = MockFilesystemReader{};
    const auto dummyLogDirPath = "/path/to/log/dir";
    const auto overrideConfigPath = "/path/to/override/config.ini";
    const char *argv[] = {"--log-dir-path", dummyLogDirPath, "-c", overrideConfigPath};

    EXPECT_CALL(mockFS, Call(overrideConfigPath))
        .WillOnce(Return(ByMove(std::make_unique<std::istringstream>(
            "log-dir-path = /path/that/should/be/overridden"))));

    auto config2 = Config::make(mockFS.AsStdFunction(), {sizeof(argv) / sizeof(argv[0]), argv});
    EXPECT_EQ(config2.logDirPath, dummyLogDirPath);
}
