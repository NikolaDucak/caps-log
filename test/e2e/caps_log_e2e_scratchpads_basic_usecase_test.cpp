#include "caps_log_e2e_test_fixture.hpp"

#include <ftxui/component/event.hpp>
#include <gmock/gmock.h>

namespace caps_log::test::e2e {

class CapsLogE2EScratchpadsBasicUsecaseTest : public CapsLogE2ETest {};

using namespace testing;

TEST_F(CapsLogE2EScratchpadsBasicUsecaseTest, SwitchToScratchpadView) {
    CapsLog capsLog{createTestContext({"caps-log"})};
    capsLog.onEvent(ftxui::Event::Character('s'));
    EXPECT_THAT(capsLog.render(),
                RenderedElementWithoutDatesEqual("caps_log_empty_scratchpads.txt"));
    capsLog.onEvent(ftxui::Event::Character('s'));
    EXPECT_THAT(capsLog.render(), RenderedElementWithoutDatesEqual("caps_log_empty_logs.txt"));
}

TEST_F(CapsLogE2EScratchpadsBasicUsecaseTest, SwitchToScratchpadView_LoadsScratchpads) {
    writeFile(getTestScratchpadDirectoryName() / "test_file.md", "dummy content");
    CapsLog capsLog{createTestContext({"caps-log"})};
    capsLog.onEvent(ftxui::Event::Character('s'));
    capsLog.onEvent(ftxui::Event::Character('j'));
    auto render = capsLog.render();
    EXPECT_THAT(render, RenderedElementWithoutDatesEqual(
                            "caps_log_scratchpad_view_with_dummy_content.txt"));
    EXPECT_THAT(render, ContainsRegex("dummy content"));
    EXPECT_THAT(render, ContainsRegex("test_file\\.md"));
}

TEST_F(CapsLogE2EScratchpadsBasicUsecaseTest, CreateScratchpad) {
    CapsLog capsLog{createTestContext({"caps-log"})};
    m_fakeEditor->setPathProvider(capsLog.getConfig().getLogFilePathProvider());

    capsLog.onEvent(ftxui::Event::Character('s'));
    capsLog.onEvent(ftxui::Event::Return);
    EXPECT_THAT(capsLog.render(),
                RenderedElementWithoutDatesEqual("caps_log_create_scratchpad_prompt.txt"));
    capsLog.onEvent(ftxui::Event::Character('t'));
    capsLog.onEvent(ftxui::Event::Character('e'));
    capsLog.onEvent(ftxui::Event::Character('s'));
    capsLog.onEvent(ftxui::Event::Character('t'));
    EXPECT_THAT(capsLog.render(), RenderedElementWithoutDatesEqual(
                                      "caps_log_create_scratchpad_prompt_test_name.txt"));
    m_fakeEditor->setContentToBeEdited("This is a test scratchpad content.");
    capsLog.onEvent(ftxui::Event::Return);
    EXPECT_TRUE(std::filesystem::exists(getTestScratchpadDirectoryName() / "test.md"));

    EXPECT_THAT(capsLog.render(),
                RenderedElementWithoutDatesEqual("caps_log_scratchpad_created_unselected.txt"));
    capsLog.onEvent(ftxui::Event::Character('j'));
    const auto render = capsLog.render();
    EXPECT_THAT(render,
                RenderedElementWithoutDatesEqual("caps_log_scratchpad_created_selected.txt"));
    // Preview should contain the scratchpad name and content
    EXPECT_THAT(render, ContainsRegex("This is a test scratchpad content."));
    EXPECT_THAT(render, ContainsRegex("test\\.md"));
}

TEST_F(CapsLogE2EScratchpadsBasicUsecaseTest, CreateScratchpadWithMDPrefix) {
    CapsLog capsLog{createTestContext({"caps-log"})};
    m_fakeEditor->setPathProvider(capsLog.getConfig().getLogFilePathProvider());

    capsLog.onEvent(ftxui::Event::Character('s'));
    capsLog.onEvent(ftxui::Event::Return);
    EXPECT_THAT(capsLog.render(),
                RenderedElementWithoutDatesEqual("caps_log_create_scratchpad_prompt.txt"));
    for (const auto chr : "test.md") {
        capsLog.onEvent(ftxui::Event::Character(chr));
    }
    EXPECT_THAT(capsLog.render(), RenderedElementWithoutDatesEqual(
                                      "caps_log_create_scratchpad_prompt_test_dot_md_name.txt"));
    m_fakeEditor->setContentToBeEdited("This is a test scratchpad content.");
    capsLog.onEvent(ftxui::Event::Return);

    // should not have .md.md suffix
    EXPECT_TRUE(std::filesystem::exists(getTestScratchpadDirectoryName() / "test.md"));

    EXPECT_THAT(capsLog.render(),
                RenderedElementWithoutDatesEqual("caps_log_scratchpad_created_unselected.txt"));
    capsLog.onEvent(ftxui::Event::Character('j'));
    const auto render = capsLog.render();
    EXPECT_THAT(render,
                RenderedElementWithoutDatesEqual("caps_log_scratchpad_created_selected.txt"));
    // Preview should contain the scratchpad name and content
    EXPECT_THAT(render, ContainsRegex("This is a test scratchpad content."));
    // there should be no .md.md suffix
    EXPECT_THAT(render, ContainsRegex("test\\.md"));
}

TEST_F(CapsLogE2EScratchpadsBasicUsecaseTest, DISABLED_TryToCreateScratchapdWithEmptyName) {}

TEST_F(CapsLogE2EScratchpadsBasicUsecaseTest, DISABLED_DeleteScratchpad) {}

TEST_F(CapsLogE2EScratchpadsBasicUsecaseTest, DISABLED_RenameSratchpad) {}

TEST_F(CapsLogE2EScratchpadsBasicUsecaseTest, DISABLED_RenameSratchpadToEmptyName) {}

} // namespace caps_log::test::e2e
