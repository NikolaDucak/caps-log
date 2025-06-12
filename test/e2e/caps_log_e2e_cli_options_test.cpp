#include "caps_log_e2e_test_fixture.hpp"
#include <gmock/gmock-matchers.h>

// CLi and config options tests

namespace caps_log::test::e2e {

class CapsLogE2ECliOptionsTest : public CapsLogE2ETest {};

TEST_F(CapsLogE2ECliOptionsTest, SundayStartFlag) {
    auto sundayStartRender = [this] {
        auto args = std::vector<std::string>{"caps-log", "--sunday-start"};
        CapsLog capsLog{createTestContext(args)};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_sunday_start_true.txt"));
        return render;
    }();
    auto mondayStartRender = [this] {
        auto args = std::vector<std::string>{"caps-log"};
        CapsLog capsLog{createTestContext(args)};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_sunday_start_false.txt"));
        return render;
    }();

    // Check that the renders are different
    EXPECT_NE(sundayStartRender, mondayStartRender)
        << "The renders for Sunday start and Monday start should be different.";
}

TEST_F(CapsLogE2ECliOptionsTest, LogDirPath) {
    auto path1 = [this]() {
        CapsLog capsLog{createTestContext({"caps-log"})};
        return capsLog.getConfig().getLogFilePathProvider().path(kToday);
    }();

    static const std::filesystem::path kTestLogDirectory2 = kTestDirectoryBase / "test_dir_two";
    auto path2 = [this]() {
        auto context = createTestContext({"caps-log"});
        // Reset CLI args as they override the config settings and test context relies on CLI args
        context.cliArgs = {"caps-log"};
        context.cliArgs.push_back("--config");
        context.cliArgs.push_back(kTestConfigPath.string());
        if (not std::filesystem::exists(kTestConfigPath)) {
            writeFile(kTestConfigPath, "");
        }
        context.cliArgs.push_back("--log-dir-path");
        context.cliArgs.push_back(kTestLogDirectory2.string());
        CapsLog capsLog{context};
        return capsLog.getConfig().getLogFilePathProvider().path(kToday);
    }();

    ASSERT_NE(path1, path2) << "The paths for different log directories should be different.";

    writeFile(path1, "dummy content for path1");
    writeFile(path2, "dummy content for path2");

    auto renderWithDefaultLogDir = [this] {
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_default_log_dir.txt"));
        EXPECT_THAT(render, testing::ContainsRegex("dummy content for path1"));
        return render;
    }();

    auto renderWithCustomLogDir = [this] {
        auto context = createTestContext({"caps-log"});
        // Reset CLI args as they override the config settings and test context relies on CLI args
        context.cliArgs = {"caps-log"};
        context.cliArgs.push_back("--config");
        context.cliArgs.push_back(kTestConfigPath.string());
        context.cliArgs.push_back("--log-dir-path");
        context.cliArgs.push_back(kTestLogDirectory2.string());
        if (not std::filesystem::exists(kTestConfigPath)) {
            writeFile(kTestConfigPath, "");
        }
        CapsLog capsLog{context};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_custom_log_dir.txt"));
        EXPECT_THAT(render, testing::ContainsRegex("dummy content for path2"));
        return render;
    }();

    EXPECT_NE(renderWithDefaultLogDir, renderWithCustomLogDir)
        << "The renders for default and custom log directories should be different.";
}

TEST_F(CapsLogE2ECliOptionsTest, LogFilenameFormat) {
    static const std::string kDifferentFileNameFormat = "diff-d%m-%d-%Y.md";
    auto pathOfLogWithDifferentFileNameFormat = [this]() {
        CapsLog capsLog{
            createTestContext({"caps-log", "--log-name-format", kDifferentFileNameFormat})};
        return capsLog.getConfig().getLogFilePathProvider().path(kToday);
    }();
    auto pathOfLogWithDefaultFileNameFormat = [this]() {
        CapsLog capsLog{createTestContext({"caps-log"})};
        return capsLog.getConfig().getLogFilePathProvider().path(kToday);
    }();

    ASSERT_NE(pathOfLogWithDifferentFileNameFormat, pathOfLogWithDefaultFileNameFormat)
        << "The paths for different and default log filename formats should be different.";

    writeFile(pathOfLogWithDifferentFileNameFormat,
              "dummy content for pathOfLogWithDifferentFileNameFormat");
    writeFile(pathOfLogWithDefaultFileNameFormat,
              "dummy content for pathOfLogWithDefaultFileNameFormat");

    auto renderWithDefaultFilenameFormat = [this] {
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_log_filename_format_default.txt"));
        EXPECT_THAT(render,
                    testing::ContainsRegex("dummy content for pathOfLogWithDefaultFileNameFormat"));
        return render;
    }();

    auto renderWithDifferentFilenameFormat = [this] {
        auto context =
            createTestContext({"caps-log", "--log-name-format", kDifferentFileNameFormat});
        CapsLog capsLog{context};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_log_filename_format_custom.txt"));
        EXPECT_THAT(render, testing::ContainsRegex(
                                "dummy content for pathOfLogWithDifferentFileNameFormat"));
        return render;
    }();

    EXPECT_NE(renderWithDefaultFilenameFormat, renderWithDifferentFilenameFormat)
        << "The renders for default and custom log directories should be different.";
}

TEST_F(CapsLogE2ECliOptionsTest, FirstLineSection) {
    auto context = createTestContext({"caps-log"});
    {
        CapsLog capsLog{context};
        writeFile(capsLog.getConfig().getLogFilePathProvider().path(kToday), "# Section one");
    }
    auto renderWithFirstLineSectionDisabled = [this, context] {
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_first_line_section_false.txt"));
        EXPECT_THAT(render, Not(testing::ContainsRegex("1 │ section one")));
        return render;
    }();
    auto renderWithFirstLineSectionEnabled = [this, context] {
        CapsLog capsLog{createTestContext({"caps-log", "--first-line-section"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_first_line_section_true.txt"));
        EXPECT_THAT(render, testing::ContainsRegex("1 │ section one"));
        return render;
    }();

    // Check that the renders are different
    EXPECT_NE(renderWithFirstLineSectionDisabled, renderWithFirstLineSectionEnabled)
        << "The renders for first line section ignored and enabled should be different.";
}

TEST_F(CapsLogE2ECliOptionsTest, DISABLED_EncryptDecrypt) {}

TEST_F(CapsLogE2ECliOptionsTest, DISABLED_Password) {}

} // namespace caps_log::test::e2e
