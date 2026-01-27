#include "caps_log_e2e_test_fixture.hpp"
#include <gmock/gmock-matchers.h>

// CLi and config options tests

namespace caps_log::test::e2e {

class CapsLogE2EConfigFileOptionsTest : public CapsLogE2ETest {};

TEST_F(CapsLogE2EConfigFileOptionsTest, SundayStartFlag) {
    auto sundayStartRender = [this] {
        writeFile(kTestConfigPath, "sunday-start=true");
        CapsLog capsLog{createTestContext({"caps-log"})};
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
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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

TEST_F(CapsLogE2EConfigFileOptionsTest, ThemeConfigInvalidThrows) {
    const auto *content = R"(
[view.annual-view.theme.event-date]
fgcolor=ansi256(999)
  )";
    writeFile(kTestConfigPath, content);

    EXPECT_THROW((CapsLog{createTestContext({"caps-log"})}), caps_log::ConfigParsingException);
}

TEST_F(CapsLogE2EConfigFileOptionsTest, ThemeConfigChangesRenderOutput) {
    auto renderDefault = [this] {
        writeFile(kTestConfigPath, "");
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_theme_default.txt"));
        return render;
    }();

    const auto *content = R"(
[view.annual-view.theme.log-date]
fgcolor=rgb(12,34,56)
bold=true
[view.annual-view.theme.weekend-date]
fgcolor=ansi256(196)
underlined=true
[view.annual-view.theme.todays-date]
fgcolor=ansi16(brightcyan)
italic=true
  )";
    auto renderThemed = [this, content] {
        writeFile(kTestConfigPath, content);
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_theme_custom.txt"));
        return render;
    }();

    EXPECT_NE(renderDefault, renderThemed) << "Themed render should differ from default render.";
}

TEST_F(CapsLogE2EConfigFileOptionsTest, ThemeBorderConfigChangesRenderOutput) {
    auto renderDefault = [this] {
        writeFile(kTestConfigPath, "");
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_theme_borders_default.txt"));
        return render;
    }();

    const auto *content = R"(
[view.annual-view.theme]
calendar-border=double
calendar-month-border=light
tags-menu.border=heavy
sections-menu.border=dashed
events-list.border=empty
log-entry-preview.border=rounded
  )";
    auto renderThemed = [this, content] {
        writeFile(kTestConfigPath, content);
        CapsLog capsLog{createTestContext({"caps-log"})};
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_theme_borders_custom.txt"));
        return render;
    }();

    EXPECT_NE(renderDefault, renderThemed)
        << "Border themed render should differ from default render.";
}

TEST_F(CapsLogE2EConfigFileOptionsTest, ThemeBorderConfigSubsectionsChangeRenderOutput) {
    const auto *content = R"(
[view.annual-view.theme.calendar-border]
border=double
[view.annual-view.theme.calendar-month-border]
border=light
[view.annual-view.theme.tags-menu]
border=heavy
[view.annual-view.theme.sections-menu]
border=dashed
[view.annual-view.theme.events-list]
border=empty
[view.annual-view.theme.log-entry-preview]
border=rounded
  )";
    writeFile(kTestConfigPath, content);

    CapsLog capsLog{createTestContext({"caps-log"})};
    auto render = capsLog.render();
    EXPECT_THAT(render, RenderedElementEqual("caps_log_theme_borders_subsections.txt"));
}

TEST_F(CapsLogE2EConfigFileOptionsTest, ThemeBorderConfigInvalidThrows) {
    const auto *content = R"(
[view.annual-view.theme]
calendar-border=not-a-border
  )";
    writeFile(kTestConfigPath, content);

    EXPECT_THROW((CapsLog{createTestContext({"caps-log"})}), caps_log::ConfigParsingException);
}

TEST_F(CapsLogE2EConfigFileOptionsTest, MarkdownThemeConfigChangesRenderOutput) {
    const auto logContent = std::string{"# Header one\n"
                                        "## Header two\n"
                                        "> quoted line\n"
                                        "- list item\n"
                                        "`inline code`\n"};
    auto renderDefault = [this] {
        writeFile(kTestConfigPath, "");
        CapsLog capsLog{createTestContext({"caps-log"})};
        writeFile(capsLog.getConfig().getLogFilePathProvider().path(kToday),
                  "# Header one\n## Header two\n> quoted line\n- list item\n`inline code`\n");
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_markdown_theme_default.txt"));
        return render;
    }();

    const auto *content = R"(
[view.annual-view.theme.log-entry-preview.markdown-theme]
header1=ansi256(200)
header2=ansi256(150)
header3=ansi256(115)
header4=ansi256(110)
header5=ansi256(127)
header6=ansi256(109)
list=ansi16(white)
quote=ansi16(magenta)
code-fg=ansi16(black)
  )";
    auto renderThemed = [this, content, logContent] {
        writeFile(kTestConfigPath, content);
        CapsLog capsLog{createTestContext({"caps-log"})};
        writeFile(capsLog.getConfig().getLogFilePathProvider().path(kToday), logContent);
        auto render = capsLog.render();
        EXPECT_THAT(render, RenderedElementEqual("caps_log_markdown_theme_custom.txt"));
        return render;
    }();

    EXPECT_NE(renderDefault, renderThemed)
        << "Markdown themed render should differ from default render.";
}

} // namespace caps_log::test::e2e
