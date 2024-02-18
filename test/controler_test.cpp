#include "app.hpp"
#include "mocks.hpp"

#include "log/log_file.hpp"
#include "view/input_handler.hpp"

#include <ftxui/component/event.hpp>
#include <gmock/gmock-spec-builders.h>
#include <memory>

using namespace caps_log;
using namespace testing;

class ControllerTest : public testing::Test {
  protected:
    // capsLog collects info for the date::Date::getToday().year
    // so collected data will miss info for other year
    // this is not very obvious when testing, resulting in failing
    // asserting some data is added if this fact is forgotten.
    // TODO: Maybe pass year as a constructor param?
    std::chrono::year_month_day selectedDate{utils::date::getToday().year(), // NOLINT
                                             std::chrono::month{5},          // NOLINT
                                             std::chrono::day{25}};          // NOLINT
    // NOLINTNEXTLINE
    std::chrono::year_month_day dayAfterSelectedDate{
        selectedDate.year(), selectedDate.month(),
        std::chrono::day{unsigned(selectedDate.day()) + 1}};
    std::shared_ptr<NiceMock<DMockYearView>> mock_view = // NOLINT
        std::make_shared<NiceMock<DMockYearView>>();
    // NOLINTNEXTLINE
    std::shared_ptr<NiceMock<DMockRepo>> mock_repo = std::make_shared<NiceMock<DMockRepo>>();
    std::shared_ptr<MockEditor> mock_editor = std::make_shared<MockEditor>(); // NOLINT

  public:
    ControllerTest() { mock_view->getDummyView().m_focusedDate = selectedDate; }
};

TEST_F(ControllerTest, EscQuits) {
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_view, stop());
        capsLog.handleInputEvent(UIEvent{UIEvent::RootEvent, ftxui::Event::Escape.input()});
    });
    capsLog.run();
}

TEST_F(ControllerTest, SpecialCharsDontQuit) {
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        ON_CALL(*mock_view, stop).WillByDefault([] {
            ASSERT_FALSE(true) << "Expected to not quit.";
        });
        capsLog.handleInputEvent(UIEvent{UIEvent::RootEvent, ftxui::Event::ArrowDown.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::RootEvent, ftxui::Event::ArrowUp.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::RootEvent, ftxui::Event::ArrowLeft.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::RootEvent, ftxui::Event::ArrowRight.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::RootEvent, ftxui::Event::Tab.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::RootEvent, ftxui::Event::TabReverse.input()});
    });

    capsLog.run();
}

TEST_F(ControllerTest, RemoveLog_PromptsThenUpdatesSectionsTagsAndMaps) {
    mock_repo->write(LogFile{selectedDate, "# DummyContent \n# Dummy Section\n* Dummy Tag"});
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};
    selectedDate = mock_view->getFocusedDate();

    // Expect dummy data has been propagated to view
    // ignoring first item '-----' that represents nothing
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 2);
    ASSERT_EQ(mock_view->getDummyView().m_tagMenuItems.size(), 2);
    EXPECT_EQ(mock_view->getDummyView().m_sectionMenuItems[1],
              caps_log::view::makeMenuItemTitle("dummy section", 1));
    EXPECT_EQ(mock_view->getDummyView().m_tagMenuItems[1],
              caps_log::view::makeMenuItemTitle("dummy tag", 1));

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_view, prompt(_, _));
        ON_CALL(*mock_view, prompt).WillByDefault([&](auto, auto callback) {
            // trigger callback as if user clicked 'yes'
            callback();
        });
        capsLog.handleInputEvent(UIEvent{UIEvent::RootEvent, "d"});
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
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};
    ASSERT_EQ(mock_view->getDummyView().m_previewString, dummyLog1.getContent());
    ASSERT_EQ(mock_view->getDummyView().m_focusedDate, dummyLog1.getDate());

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        mock_view->getDummyView().m_focusedDate = dummyLog2.getDate();
        capsLog.handleInputEvent(UIEvent{UIEvent::FocusedDateChange, ""});
        ASSERT_EQ(mock_view->getDummyView().m_previewString, dummyLog2.getContent());
    });

    capsLog.run();
}

TEST_F(ControllerTest, OnSelectedMenuItemChange_UpdateHighlightMap) {
    auto dummyLog1 = LogFile{selectedDate, "\n# sectone \n* tagone"};
    auto dummyLog2 = LogFile{dayAfterSelectedDate, "\n# secttwo \n* tagtwo"};
    mock_repo->getDummyRepo().write(dummyLog1);
    mock_repo->getDummyRepo().write(dummyLog2);
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&]() { // NOLINT
        // Tags and sections are passed from the view as an index in the section/tagMenuItem vector
        // 0 = '------' aka nothing selected
        capsLog.handleInputEvent(UIEvent{UIEvent::FocusedTagChange, "1"});
        auto *dummyView = &mock_view->getDummyView();

        // Tags
        ASSERT_NE(dummyView->m_highlightedLogsMap, nullptr);
        ASSERT_TRUE(dummyView->m_highlightedLogsMap->get(dummyLog1.getDate()));
        ASSERT_FALSE(dummyView->m_highlightedLogsMap->get(dummyLog2.getDate()));

        capsLog.handleInputEvent(UIEvent{UIEvent::FocusedTagChange, "2"});
        ASSERT_NE(dummyView->m_highlightedLogsMap, nullptr);
        ASSERT_FALSE(dummyView->m_highlightedLogsMap->get(dummyLog1.getDate()));
        ASSERT_TRUE(dummyView->m_highlightedLogsMap->get(dummyLog2.getDate()));

        // Sections
        capsLog.handleInputEvent(UIEvent{UIEvent::FocusedSectionChange, "1"});
        ASSERT_NE(dummyView->m_highlightedLogsMap, nullptr);
        ASSERT_TRUE(dummyView->m_highlightedLogsMap->get(dummyLog1.getDate()));
        ASSERT_FALSE(dummyView->m_highlightedLogsMap->get(dummyLog2.getDate()));

        capsLog.handleInputEvent(UIEvent{UIEvent::FocusedSectionChange, "2"});
        ASSERT_NE(dummyView->m_highlightedLogsMap, nullptr);
        ASSERT_FALSE(dummyView->m_highlightedLogsMap->get(dummyLog1.getDate()));
        ASSERT_TRUE(dummyView->m_highlightedLogsMap->get(dummyLog2.getDate()));
    });
    capsLog.run();
}

TEST_F(ControllerTest, AddLog_UpdatesSectionsTagsAndMaps) {
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};
    // +1 for '-----' aka no section
    ASSERT_EQ(mock_view->getDummyView().m_tagMenuItems.size(), 1);
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 1);
    ASSERT_NE(mock_view->getDummyView().m_availableLogsMap, nullptr);
    EXPECT_EQ(mock_view->getDummyView().m_availableLogsMap->get(selectedDate), false);

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        // expect editor to open
        EXPECT_CALL(*mock_editor, openEditor(_)).WillRepeatedly([&](auto) {
            auto log =
                LogFile{selectedDate, "\n# section title \nsome dummy content\n * tag title"};
            mock_repo->getDummyRepo().write(log);
        });

        // on callendar button click
        capsLog.handleInputEvent(UIEvent{UIEvent::CalendarButtonClick, ""});

        // expect the initialy set availability map pointer to still be valid
        EXPECT_EQ(mock_view->getDummyView().m_availableLogsMap->get(selectedDate), true);
        ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 2);
        EXPECT_EQ(mock_view->getDummyView().m_sectionMenuItems.at(1), "(1) - section title");
        ASSERT_EQ(mock_view->getDummyView().m_tagMenuItems.size(), 2);
        EXPECT_EQ(mock_view->getDummyView().m_tagMenuItems.at(1), "(1) - tag title");
    });

    capsLog.run();
}

TEST_F(ControllerTest, AddLog_WritesABaslineTemplateForEmptyLog) {
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};
    EXPECT_EQ(mock_view->getDummyView().m_availableLogsMap->get(selectedDate), false);

    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_editor, openEditor(_)).WillRepeatedly([&](auto) {
            auto baseLog = mock_repo->getDummyRepo().read(selectedDate);
            ASSERT_TRUE(baseLog);
            EXPECT_EQ(baseLog->getContent(),
                      utils::date::formatToString(selectedDate, kLogBaseTemplate));
        });
        capsLog.handleInputEvent(UIEvent{UIEvent::CalendarButtonClick, ""});
    });

    capsLog.run();
}

TEST_F(ControllerTest, AddLog_AddedEmptyLogGetsRemoved) {
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};
    EXPECT_EQ(mock_view->getDummyView().m_availableLogsMap->get(selectedDate), false);

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

        capsLog.handleInputEvent(UIEvent{UIEvent::CalendarButtonClick, ""});
        EXPECT_FALSE(mock_repo->getDummyRepo().read(selectedDate));

        capsLog.handleInputEvent(UIEvent{UIEvent::CalendarButtonClick, ""});
        EXPECT_FALSE(mock_repo->getDummyRepo().read(selectedDate));
    });
    capsLog.run();
}

TEST_F(ControllerTest, AddLog_ConfigSkipsFirstSection) {
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 1);
    mock_repo->write(LogFile{selectedDate, "# Dummy section"});
    auto capsLog2 = caps_log::App{mock_view, mock_repo, mock_editor, false};
    // +1 for '-----' aka no section
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 2);
}
