#include "app.hpp"
#include "mocks.hpp"

#include "log/log_file.hpp"
#include "utils/date.hpp"
#include "view/input_handler.hpp"

#include <ftxui/component/event.hpp>
#include <gmock/gmock-spec-builders.h>
#include <gmock/gmock.h>
#include <memory>

namespace caps_log::test {

using namespace testing;

class ControllerTest : public testing::Test {
  protected:
    // NOLINTNEXTLINE
    static constexpr std::chrono::year_month_day selectedDate{
        std::chrono::year{2000}, std::chrono::month{5}, std::chrono::day{25}};
    // NOLINTNEXTLINE
    static constexpr std::chrono::year_month_day dayAfterSelectedDate{
        selectedDate.year(), selectedDate.month(), ++selectedDate.day()};

    std::shared_ptr<NiceMock<DMockYearView>> mock_view = // NOLINT
        std::make_shared<NiceMock<DMockYearView>>();
    // NOLINTNEXTLINE
    std::shared_ptr<NiceMock<DMockRepo>> mock_repo = std::make_shared<NiceMock<DMockRepo>>();
    std::shared_ptr<MockEditor> mock_editor = std::make_shared<MockEditor>(); // NOLINT

    /**
     * @brief Create a caps_log::App object with the mock objects and selected year.
     */
    auto makeCapsLog() {
        return App{mock_view, mock_repo, mock_editor, true, std::nullopt, selectedDate.year()};
    }

  public:
    ControllerTest() { mock_view->getDummyView().m_focusedDate = selectedDate; }
};

TEST_F(ControllerTest, EscQuits) {
    auto capsLog = makeCapsLog();

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_view, stop());
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::Escape.input()}});
    });
    capsLog.run();
}

TEST_F(ControllerTest, SpecialCharsDontQuit) {
    auto capsLog = makeCapsLog();

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        ON_CALL(*mock_view, stop).WillByDefault([] {
            ASSERT_FALSE(true) << "Expected to not quit.";
        });
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::ArrowDown.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::ArrowUp.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::ArrowLeft.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::ArrowRight.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::Tab.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::TabReverse.input()}});
    });

    capsLog.run();
}

TEST_F(ControllerTest, RemoveLog_PromptsThenUpdatesSectionsTagsAndMaps) {
    mock_repo->write(LogFile{selectedDate, "# DummyContent \n# Dummy Section\n* Dummy Tag"});
    auto capsLog = makeCapsLog();

    // Expect dummy data has been propagated to view
    // ignoring first item '-----' that represents nothing
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 2);
    ASSERT_EQ(mock_view->getDummyView().m_tagMenuItems.size(), 2);
    EXPECT_EQ(mock_view->getDummyView().m_sectionMenuItems[1],
              view::makeMenuItemTitle("dummy section", 1));
    EXPECT_EQ(mock_view->getDummyView().m_tagMenuItems[1], view::makeMenuItemTitle("dummy tag", 1));

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_view, prompt(_, _));
        ON_CALL(*mock_view, prompt).WillByDefault([&](auto, auto callback) {
            // trigger callback as if user clicked 'yes'
            callback();
        });
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{"d"}});
        // assert that it has indeed been removed
        EXPECT_FALSE(mock_repo->read(selectedDate));
        // and that the view things have been updated
        EXPECT_EQ(mock_view->getDummyView().m_tagMenuItems.size(), 1);
        EXPECT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 1);
    });
    capsLog.run();
}

TEST_F(ControllerTest, OnFocusedDateChange_UpdatePreviewString) {
    auto dummyLog1 = LogFile{selectedDate, "dummy content 1"};
    auto dummyLog2 = LogFile{dayAfterSelectedDate, "dummy content 2"};
    mock_repo->write(dummyLog1);
    mock_repo->write(dummyLog2);
    auto capsLog = makeCapsLog();
    ASSERT_EQ(mock_view->getDummyView().m_previewString, dummyLog1.getContent());
    ASSERT_EQ(mock_view->getDummyView().m_focusedDate, dummyLog1.getDate());

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        mock_view->getDummyView().m_focusedDate = dummyLog2.getDate();
        capsLog.handleInputEvent(UIEvent{FocusedDateChange{dummyLog2.getDate()}});
        ASSERT_EQ(mock_view->getDummyView().m_previewString, dummyLog2.getContent());
    });

    capsLog.run();
}

TEST_F(ControllerTest, OnSelectedMenuItemChange_UpdateHighlightMap) {
    auto dummyLog1 = LogFile{selectedDate, "\n# sectone \n* tagone"};
    auto dummyLog2 = LogFile{dayAfterSelectedDate, "\n# secttwo \n* tagtwo"};
    mock_repo->getDummyRepo().write(dummyLog1);
    mock_repo->getDummyRepo().write(dummyLog2);
    auto capsLog = makeCapsLog();

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&]() { // NOLINT
        // Tags and sections are passed from the view as an index in the section/tagMenuItem
        // vector 0 = '------' aka nothing selected
        capsLog.handleInputEvent(UIEvent{FocusedTagChange{1}});
        auto *dummyView = &mock_view->getDummyView();

        using utils::date::monthDay;
        // Tags
        ASSERT_NE(dummyView->m_highlightedDates, nullptr);
        ASSERT_TRUE(dummyView->m_highlightedDates->contains(monthDay(dummyLog1.getDate())));
        ASSERT_FALSE(dummyView->m_highlightedDates->contains(monthDay(dummyLog2.getDate())));

        capsLog.handleInputEvent(UIEvent{FocusedTagChange{2}});
        ASSERT_NE(dummyView->m_highlightedDates, nullptr);
        ASSERT_FALSE(dummyView->m_highlightedDates->contains(monthDay(dummyLog1.getDate())));
        ASSERT_TRUE(dummyView->m_highlightedDates->contains(monthDay(dummyLog2.getDate())));

        // Sections
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{1}});
        ASSERT_NE(dummyView->m_highlightedDates, nullptr);
        ASSERT_TRUE(dummyView->m_highlightedDates->contains(monthDay(dummyLog1.getDate())));
        ASSERT_FALSE(dummyView->m_highlightedDates->contains(monthDay(dummyLog2.getDate())));

        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{2}});
        ASSERT_NE(dummyView->m_highlightedDates, nullptr);
        ASSERT_FALSE(dummyView->m_highlightedDates->contains(monthDay(dummyLog1.getDate())));
        ASSERT_TRUE(dummyView->m_highlightedDates->contains(monthDay(dummyLog2.getDate())));
    });
    capsLog.run();
}

TEST_F(ControllerTest, AddLog_UpdatesSectionsTagsAndMaps) {
    auto capsLog = makeCapsLog();
    // +1 for '-----' aka no section
    ASSERT_EQ(mock_view->getDummyView().m_tagMenuItems.size(), 1);
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 1);
    ASSERT_NE(mock_view->getDummyView().m_datesWithLogs, nullptr);
    EXPECT_FALSE(
        mock_view->getDummyView().m_datesWithLogs->contains(utils::date::monthDay(selectedDate)));

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        // expect editor to open
        EXPECT_CALL(*mock_editor, openEditor(_)).WillRepeatedly([&](auto) {
            auto log =
                LogFile{selectedDate, "\n# section title \nsome dummy content\n * tag title"};
            mock_repo->getDummyRepo().write(log);
        });

        // on callendar button click
        capsLog.handleInputEvent(UIEvent{OpenLogFile{selectedDate}});

        // expect the initialy set availability map pointer to still be valid
        EXPECT_TRUE(mock_view->getDummyView().m_datesWithLogs->contains(
            utils::date::monthDay(selectedDate)));
        ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 2);
        EXPECT_EQ(mock_view->getDummyView().m_sectionMenuItems.at(1), "(1) - section title");
        ASSERT_EQ(mock_view->getDummyView().m_tagMenuItems.size(), 2);
        EXPECT_EQ(mock_view->getDummyView().m_tagMenuItems.at(1), "(1) - tag title");
    });

    capsLog.run();
}

TEST_F(ControllerTest, AddLog_WritesABaslineTemplateForEmptyLog) {
    auto capsLog = makeCapsLog();
    EXPECT_FALSE(
        mock_view->getDummyView().m_datesWithLogs->contains(utils::date::monthDay(selectedDate)));

    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_editor, openEditor(_)).WillRepeatedly([&](auto) {
            auto baseLog = mock_repo->getDummyRepo().read(selectedDate);
            ASSERT_TRUE(baseLog);
            EXPECT_EQ(baseLog->getContent(),
                      utils::date::formatToString(selectedDate, kLogBaseTemplate));
        });
        capsLog.handleInputEvent(UIEvent{OpenLogFile{selectedDate}});
    });

    capsLog.run();
}

TEST_F(ControllerTest, AddLog_AddedEmptyLogGetsRemoved) {
    auto capsLog = makeCapsLog();
    EXPECT_FALSE(
        mock_view->getDummyView().m_datesWithLogs->contains(utils::date::monthDay(selectedDate)));

    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_editor, openEditor(_))
            .WillOnce([&](auto) {
                // Assert only base template has been written
                auto baseLog = mock_repo->getDummyRepo().read(selectedDate);
                ASSERT_TRUE(baseLog);
                EXPECT_EQ(baseLog->getContent(),
                          utils::date::formatToString(selectedDate, kLogBaseTemplate));
            })
            .WillOnce([&](auto) {
                // Write an empty log
                mock_repo->getDummyRepo().write({selectedDate, ""});
            });

        capsLog.handleInputEvent(UIEvent{OpenLogFile{selectedDate}});
        EXPECT_FALSE(mock_repo->getDummyRepo().read(selectedDate));

        capsLog.handleInputEvent(UIEvent{OpenLogFile{selectedDate}});
        EXPECT_FALSE(mock_repo->getDummyRepo().read(selectedDate));
    });
    capsLog.run();
}

TEST_F(ControllerTest, AddLog_ConfigSkipsFirstSection) {
    auto capsLog = makeCapsLog();
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 1);
    mock_repo->write(LogFile{selectedDate, "# Dummy section"});
    auto capsLog2 =
        App{mock_view, mock_repo, mock_editor, false, std::nullopt, selectedDate.year()};

    // +1 for '-----' aka no section
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 2);
}

} // namespace caps_log::test
