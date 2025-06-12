#include "caps_log_e2e_test_fixture.hpp"

#include <ftxui/component/event.hpp>
#include <gmock/gmock.h>
#include <regex>

namespace caps_log::test::e2e {

class CapsLogE2ELogsBasicUsecaseTest : public CapsLogE2ETest {};

using namespace testing;

TEST_F(CapsLogE2ELogsBasicUsecaseTest, DoNotKeeprenderFileUpdateFlagToTrue) {
    ASSERT_FALSE(kRefreshDataFiles)
        << "This flag should not be set to true, as it is only used for development purposes.";
}

TEST_F(CapsLogE2ELogsBasicUsecaseTest, BasicStartAndQuit) {
    // Create a CapsLog instance with mock objects
    auto args = std::vector<std::string>{"caps-log"};
    CapsLog capsLog{createTestContext(args)};

    const auto task = capsLog.getTask();
    ASSERT_EQ(task.type, CapsLog::Task::Type::kRunAppplication);

    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_basic_start_and_quit.txt"));
}

TEST_F(CapsLogE2ELogsBasicUsecaseTest, TabCyclesThroughAllElementsOfTheView) {
    // Create a CapsLog instance with mock objects
    auto args = std::vector<std::string>{"caps-log"};
    CapsLog capsLog{createTestContext(args)};

    capsLog.onEvent(ftxui::Event::Tab);
    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_tab_cycle_1_tags.txt"));
    capsLog.onEvent(ftxui::Event::Tab);
    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_tab_cycle_2_calendar.txt"));
    capsLog.onEvent(ftxui::Event::Tab);
    // TODO: this actualy selects the next element, because there are no events
    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_tab_cycle_3_events.txt"));
    capsLog.onEvent(ftxui::Event::Tab);
    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_tab_cycle_4_preview.txt"));
}

std::optional<std::string> readFile(const std::filesystem::path &filePath) {

    if (not std::filesystem::exists(filePath)) {
        return std::nullopt; // File does not exist
    }
    std::ifstream file{filePath};

    if (!file.is_open()) {
        throw std::runtime_error{"Failed to open file: " + filePath.string()};
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

TEST_F(CapsLogE2ELogsBasicUsecaseTest, OpeningAndWritingALogForToday_UpdatesTheView) {
    // Create a CapsLog instance with mock objects
    auto args = std::vector<std::string>{"caps-log"};
    CapsLog capsLog{createTestContext(args)};

    capsLog.onEvent(ftxui::Event::Tab);
    capsLog.onEvent(ftxui::Event::Tab);

    std::string logContent = "This is a test log for today.\n# Test Section one\n* Test tag one";
    editorWillWriteLog(logContent, capsLog.getConfig().getLogFilePathProvider());
    capsLog.onEvent(ftxui::Event::Return);

    // Check if the log was written and the view was updated
    auto fileContent = readFile(capsLog.getConfig().getLogFilePathProvider().path(kToday));
    ASSERT_TRUE(fileContent) << "Log file was not created";
    ASSERT_EQ(fileContent, logContent) << "Log content does not match expected content.";

    auto render = capsLog.render();
    EXPECT_THAT(render, RenderedElementEqual("caps_log_open_and_write_log.txt"));
    // extra check. ensure "test tag one" is somewhere in the rendered output, "test section one"
    // too, and LogContent is there too. But first remove non alphanumeric characters
    auto filteredrender = std::regex_replace(render, std::regex("[^a-zA-Z0-9 ]"), "");
    EXPECT_THAT(filteredrender, ContainsRegex("test tag one"));
    EXPECT_THAT(filteredrender,
                ContainsRegex("test section on")); // the 'e' is not shown as it is too long
    EXPECT_THAT(filteredrender, ContainsRegex("This is a test log for today."));
}

TEST_F(CapsLogE2ELogsBasicUsecaseTest, DeletingALogForToday_Prompts_ThenUpdatesTheView) {
    auto args = std::vector<std::string>{"caps-log"};
    auto context = createTestContext(args);

    {
        // Create a caps log instance to get config. Since caps log does not update loaded log files
        // in the view if they are not changed through the editor, we need to write a log file
        // for today and recreate the caps log instance.
        CapsLog capsLog{context};
        std::string logContent =
            "This is a test log for today.\n# Test Section one\n* Test tag one";
        writeFile(capsLog.getConfig().getLogFilePathProvider().path(kToday), logContent);
    }

    CapsLog capsLog{context};

    capsLog.onEvent(ftxui::Event::Tab);
    capsLog.onEvent(ftxui::Event::Tab);
    capsLog.onEvent(ftxui::Event::Character('d'));
    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_delete_log_prompt.txt"));
    capsLog.onEvent(ftxui::Event::Return);
    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_delete_log.txt"));
    // Extra check: ensure there are no mentions of the log
    auto render = capsLog.render();
    auto filteredrender = std::regex_replace(render, std::regex("[^a-zA-Z0-9 ]"), "");
    EXPECT_THAT(filteredrender, Not(ContainsRegex("test tag one")));
    EXPECT_THAT(filteredrender,
                Not(ContainsRegex("test section on"))); // the 'e' is not shown as it is too long
    EXPECT_THAT(filteredrender, Not(ContainsRegex("This is a test log for today.")));
}

TEST_F(CapsLogE2ELogsBasicUsecaseTest, SwitchToScratchpadView) {
    auto args = std::vector<std::string>{"caps-log"};
    CapsLog capsLog{createTestContext(args)};

    capsLog.onEvent(ftxui::Event::Character('s'));
    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_switch_to_scratchpad.txt"));
    capsLog.onEvent(ftxui::Event::Character('s'));
    EXPECT_THAT(capsLog.render(), RenderedElementEqual("caps_log_switch_to_calendar.txt"));
}

} // namespace caps_log::test::e2e
