#include "app.hpp"
#include "mocks.hpp"

#include "log/log_file.hpp"
#include "log/log_repository_base.hpp"
#include "log/year_overview_data.hpp"
#include "view/input_handler.hpp"
#include "view/year_view_base.hpp"

#include <fstream>
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
    date::Date selectedDate{25, 5, date::Date::getToday().year};
    date::Date dayAfterSelectedDate{26, 5, date::Date::getToday().year};
    std::shared_ptr<NiceMock<DMockYearView>> mock_view =
        std::make_shared<NiceMock<DMockYearView>>();
    std::shared_ptr<NiceMock<DMockRepo>> mock_repo = std::make_shared<NiceMock<DMockRepo>>();
    std::shared_ptr<MockEditor> mock_editor = std::make_shared<MockEditor>();

  public:
    ControllerTest() { mock_view->getDummyView().m_focusedDate = selectedDate; }
};

TEST_F(ControllerTest, EscQuits) {
    caps_log::App capsLog{mock_view, mock_repo, mock_editor};

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_view, stop());
        capsLog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, ftxui::Event::Escape.input()});
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
        capsLog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, ftxui::Event::ArrowDown.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, ftxui::Event::ArrowUp.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, ftxui::Event::ArrowLeft.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, ftxui::Event::ArrowRight.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, ftxui::Event::Tab.input()});
        capsLog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, ftxui::Event::TabReverse.input()});
    });

    capsLog.run();
}

TEST_F(ControllerTest, RemoveLog_PromptsThenUpdatesSectionsTagsAndMaps) {
    mock_repo->write(LogFile{selectedDate, "# DummyContent \n# Dummy Section\n* Dummy Tag"});
    auto capsLog = caps_log::App{mock_view, mock_repo, mock_editor};
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
        ON_CALL(*mock_view, prompt).WillByDefault([&](auto, auto cb) {
            // trigger callback as if user clicked 'yes'
            cb();
        });
        capsLog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, "d"});
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
    auto dummyLog2 = LogFile{{10, 10, 2005}, "dummy content 2"};
    mock_repo->write(dummyLog1);
    mock_repo->write(dummyLog2);
    auto capsLog = caps_log::App{mock_view, mock_repo, mock_editor};
    ASSERT_EQ(mock_view->getDummyView().m_previewString, dummyLog1.getContent());
    ASSERT_EQ(mock_view->getDummyView().m_focusedDate, dummyLog1.getDate());

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        mock_view->getDummyView().m_focusedDate = dummyLog2.getDate();
        capsLog.handleInputEvent(UIEvent{UIEvent::FOCUSED_DATE_CHANGE, ""});
        ASSERT_EQ(mock_view->getDummyView().m_previewString, dummyLog2.getContent());
    });

    capsLog.run();
}

TEST_F(ControllerTest, OnSelectedMenuItemChange_UpdateHighlightMap) {
    auto dummyLog1 = LogFile{selectedDate, "\n# sectone \n* tagone"};
    auto dummyLog2 = LogFile{dayAfterSelectedDate, "\n# secttwo \n* tagtwo"};
    mock_repo->getDummyRepo().write(dummyLog1);
    mock_repo->getDummyRepo().write(dummyLog2);
    auto capsLog = caps_log::App{mock_view, mock_repo, mock_editor};

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&]() {
        // Tags and sections are passed from the view as an index in the section/tagMenuItem vector
        // 0 = '------' aka nothing selected
        capsLog.handleInputEvent(UIEvent{UIEvent::FOCUSED_TAG_CHANGE, "1"});
        auto *dummy_view = &mock_view->getDummyView();

        // Tags
        ASSERT_NE(dummy_view->m_highlightedLogsMap, nullptr);
        ASSERT_TRUE(dummy_view->m_highlightedLogsMap->get(dummyLog1.getDate()));
        ASSERT_FALSE(dummy_view->m_highlightedLogsMap->get(dummyLog2.getDate()));

        capsLog.handleInputEvent(UIEvent{UIEvent::FOCUSED_TAG_CHANGE, "2"});
        ASSERT_NE(dummy_view->m_highlightedLogsMap, nullptr);
        ASSERT_FALSE(dummy_view->m_highlightedLogsMap->get(dummyLog1.getDate()));
        ASSERT_TRUE(dummy_view->m_highlightedLogsMap->get(dummyLog2.getDate()));

        // Sections
        capsLog.handleInputEvent(UIEvent{UIEvent::FOCUSED_SECTION_CHANGE, "1"});
        ASSERT_NE(dummy_view->m_highlightedLogsMap, nullptr);
        ASSERT_TRUE(dummy_view->m_highlightedLogsMap->get(dummyLog1.getDate()));
        ASSERT_FALSE(dummy_view->m_highlightedLogsMap->get(dummyLog2.getDate()));

        capsLog.handleInputEvent(UIEvent{UIEvent::FOCUSED_SECTION_CHANGE, "2"});
        ASSERT_NE(dummy_view->m_highlightedLogsMap, nullptr);
        ASSERT_FALSE(dummy_view->m_highlightedLogsMap->get(dummyLog1.getDate()));
        ASSERT_TRUE(dummy_view->m_highlightedLogsMap->get(dummyLog2.getDate()));
    });
    capsLog.run();
}

TEST_F(ControllerTest, AddLog_UpdatesSectionsTagsAndMaps) {
    auto capsLog = caps_log::App{mock_view, mock_repo, mock_editor};
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
        capsLog.handleInputEvent(UIEvent{UIEvent::CALENDAR_BUTTON_CLICK, ""});

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
    auto capsLog = caps_log::App{mock_view, mock_repo, mock_editor};
    EXPECT_EQ(mock_view->getDummyView().m_availableLogsMap->get(selectedDate), false);

    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_editor, openEditor(_)).WillRepeatedly([&](auto) {
            auto baseLog = mock_repo->getDummyRepo().read(selectedDate);
            ASSERT_TRUE(baseLog);
            EXPECT_EQ(baseLog->getContent(), selectedDate.formatToString(LOG_BASE_TEMPLATE));
        });
        capsLog.handleInputEvent(UIEvent{UIEvent::CALENDAR_BUTTON_CLICK, ""});
    });

    capsLog.run();
}

TEST_F(ControllerTest, AddLog_AddedEmptyLogGetsRemoved) {
    auto capsLog = caps_log::App{mock_view, mock_repo, mock_editor};
    EXPECT_EQ(mock_view->getDummyView().m_availableLogsMap->get(selectedDate), false);

    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_editor, openEditor(_))
            .WillOnce([&](auto) {
                // Assert only base template has been written
                auto baseLog = mock_repo->getDummyRepo().read(selectedDate);
                ASSERT_TRUE(baseLog);
                EXPECT_EQ(baseLog->getContent(), selectedDate.formatToString(LOG_BASE_TEMPLATE));
            })
            .WillOnce([&](auto) {
                // Write an empty log
                mock_repo->getDummyRepo().write({selectedDate, ""});
            });

        capsLog.handleInputEvent(UIEvent{UIEvent::CALENDAR_BUTTON_CLICK, ""});
        EXPECT_FALSE(mock_repo->getDummyRepo().read(selectedDate));

        capsLog.handleInputEvent(UIEvent{UIEvent::CALENDAR_BUTTON_CLICK, ""});
        EXPECT_FALSE(mock_repo->getDummyRepo().read(selectedDate));
    });
    capsLog.run();
}

TEST_F(ControllerTest, AddLog_ConfigSkipsFirstSection) {
    auto capsLog = caps_log::App{mock_view, mock_repo, mock_editor, true};
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 1);
    mock_repo->write(LogFile{selectedDate, "# Dummy section"});
    capsLog = caps_log::App{mock_view, mock_repo, mock_editor, false};
    // +1 for '-----' aka no section
    ASSERT_EQ(mock_view->getDummyView().m_sectionMenuItems.size(), 2);
}
