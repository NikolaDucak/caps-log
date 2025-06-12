#include "caps_log_e2e_test_fixture.hpp"
#include <gmock/gmock-matchers.h>

// CLi and config options tests

namespace caps_log::test::e2e {

class CapsLogE2EConfigFileOptionsTest : public CapsLogE2ETest {};

TEST_F(CapsLogE2EConfigFileOptionsTest, SundayStartFlag) {
    auto sundayStartRender = [this] {
        writeFile(kTestConfigPath, "sunday-start=true");
        CapsLog capsLog{createTestContext({"caps-log"})};
        capsLog.getConfig();
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_sunday_start_true.txt"));
        return render;
    }();
    auto mondayStartRender = [this] {
        writeFile(kTestConfigPath, "sunday-start=false");
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_sunday_start_false.txt"));
        return render;
    }();
    auto unspecifiedRender = [this] {
        CapsLog capsLog{createTestContext({"caps-log"})};
        writeFile(kTestConfigPath, "");
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_sunday_start_false.txt"));
        return render;
    }();

    // Check that the renders are different
    EXPECT_NE(sundayStartRender, mondayStartRender)
        << "The renders for Sunday start and Monday start should be different.";
}

// This test is a bit sketchy, as the e2e tests rely on log-dir-path CLI setting being overridden,
// so we are not testing the actual default behaviour
TEST_F(CapsLogE2EConfigFileOptionsTest, LogDirPath) {
    auto path1 = [this]() {
        CapsLog capsLog{createTestContext({"caps-log"})};
        return capsLog.getConfig().getLogFilePathProvider().path(kToday);
    }();

    static const std::filesystem::path kTestLogDirectory2 = kTestDirectoryBase / "test_dir_two";
    auto path2 = [this]() {
        writeFile(kTestConfigPath, "log-dir-path=" + kTestLogDirectory2.string());
        auto context = createTestContext({"caps-log"});
        // Reset CLI args as they override the config settings and test context relies on CLI args
        context.cliArgs = {"caps-log"};
        context.cliArgs.push_back("--config");
        context.cliArgs.push_back(kTestConfigPath.string());
        CapsLog capsLog{context};
        return capsLog.getConfig().getLogFilePathProvider().path(kToday);
    }();

    ASSERT_NE(path1, path2) << "The paths for different log directories should be different.";

    writeFile(path1, "dummy content for path1");
    writeFile(path2, "dummy content for path2");

    auto renderWithDefaultLogDir = [this] {
        writeFile(kTestConfigPath, "");
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_default_log_dir.txt"));
        EXPECT_THAT(render, testing::ContainsRegex("dummy content for path1"));
        return render;
    }();

    auto renderWithCustomLogDir = [this] {
        writeFile(kTestConfigPath, "log-dir-path=" + kTestLogDirectory2.string());
        auto context = createTestContext({"caps-log"});
        // Reset CLI args as they override the config settings and test context relies on CLI args
        context.cliArgs = {"caps-log"};
        context.cliArgs.push_back("--config");
        context.cliArgs.push_back(kTestConfigPath.string());
        CapsLog capsLog{context};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_custom_log_dir.txt"));
        EXPECT_THAT(render, testing::ContainsRegex("dummy content for path2"));
        return render;
    }();

    EXPECT_NE(renderWithDefaultLogDir, renderWithCustomLogDir)
        << "The renders for default and custom log directories should be different.";
}

TEST_F(CapsLogE2EConfigFileOptionsTest, LogFilenameFormat) {
    static const std::string kDifferentFileNameFormat = "diff-d%m-%d-%Y.md";
    auto pathOfLogWithDifferentFileNameFormat = [this]() {
        writeFile(kTestConfigPath, "log-filename-format=" + kDifferentFileNameFormat);
        CapsLog capsLog{createTestContext({"caps-log"})};
        return capsLog.getConfig().getLogFilePathProvider().path(kToday);
    }();
    auto pathOfLogWithDefaultFileNameFormat = [this]() {
        writeFile(kTestConfigPath, "");
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
        writeFile(kTestConfigPath, "");
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_default_log_flename_format.txt"));
        EXPECT_THAT(render,
                    testing::ContainsRegex("dummy content for pathOfLogWithDefaultFileNameFormat"));
        return render;
    }();

    auto renderWithDifferentFilenameFormat = [this] {
        writeFile(kTestConfigPath, "log-filename-format=" + kDifferentFileNameFormat);
        auto context = createTestContext({"caps-log"});
        CapsLog capsLog{context};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_custom_log_filename_format.txt"));
        EXPECT_THAT(render, testing::ContainsRegex(
                                "dummy content for pathOfLogWithDifferentFileNameFormat"));
        return render;
    }();

    EXPECT_NE(renderWithDefaultFilenameFormat, renderWithDifferentFilenameFormat)
        << "The renders for default and custom log directories should be different.";
}

TEST_F(CapsLogE2EConfigFileOptionsTest, FirstLineSection) {
    auto context = createTestContext({"caps-log"});
    {
        CapsLog capsLog{context};
        writeFile(capsLog.getConfig().getLogFilePathProvider().path(kToday), "# Section one");
    }
    auto renderWithFirstLineSectionIgnored = [this, context] {
        writeFile(kTestConfigPath, "first-line-section=false");
        CapsLog capsLog{context};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_first_line_section_false.txt"));
        EXPECT_THAT(render, Not(testing::ContainsRegex("1 │ section one")));

        return render;
    }();
    auto renderWithFirstLineSectionEnabled = [this, context] {
        writeFile(kTestConfigPath, "first-line-section=true");
        CapsLog capsLog{context};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_first_line_section_true.txt"));
        EXPECT_THAT(render, testing::ContainsRegex("1 │ section one"));
        return render;
    }();
    auto renderWithFirstLineSectionUnspecified = [this, context] {
        writeFile(kTestConfigPath, "");
        CapsLog capsLog{context};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_first_line_section_false.txt"));
        EXPECT_THAT(render, Not(testing::ContainsRegex("1 │ section one")));
        return render;
    }();

    // Check that the renders are different
    EXPECT_NE(renderWithFirstLineSectionIgnored, renderWithFirstLineSectionEnabled)
        << "The renders for first line section ignored and enabled should be different.";
}

TEST_F(CapsLogE2EConfigFileOptionsTest, CalendarEvents) {
    const auto *content = R"(
[calendar-events]
recent-events-window=10

[calendar-events.test-event-group-name.0]
name=event-for-today
date=15.06
[calendar-events.test-event-group-name.1]
name=event-from-past
date=10.06.
[calendar-events.test-event-group-name-2.0]
name=event-from-future
date=20.06.
[calendar-events.test-event-group-name-2.1]
name=event-too-far-in-future
date=31.12.
  )";

    writeFile(kTestConfigPath, content);

    CapsLog capsLog{createTestContext({"caps-log"})};
    auto render = capsLog.render();
    EXPECT_THAT(render, RenderedElementEqual("caps_log_calendar_events.txt"));

    EXPECT_THAT(render, testing::ContainsRegex("event-for-today"));
    EXPECT_THAT(render, testing::ContainsRegex("event-from-past"));
    // todays event should be in the title of the calendar view
    EXPECT_THAT(render, testing::ContainsRegex(
                            "Today is: 15. 06. 2024. test-event-group-name - event-for-today"));
    // only three events are within the recent events window: event-for-today, event-from-past,
    // event-from-future
    EXPECT_THAT(render, testing::ContainsRegex("Recent \\& upcoming events \\(3\\)"));
    EXPECT_THAT(render, Not(testing::ContainsRegex("event-too-far-in-future")));
}

} // namespace caps_log::test::e2e
