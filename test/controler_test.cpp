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
    static constexpr std::chrono::year_month_day day1{std::chrono::year{2000},
                                                      std::chrono::month{5}, std::chrono::day{10}};
    // NOLINTNEXTLINE
    static constexpr std::chrono::year_month_day day2{std::chrono::year{2000},
                                                      std::chrono::month{5}, std::chrono::day{11}};
    // NOLINTNEXTLINE
    static constexpr std::chrono::year_month_day day3{std::chrono::year{2000},
                                                      std::chrono::month{5}, std::chrono::day{12}};

    std::shared_ptr<NiceMock<DMockYearView>> mockView = // NOLINT
        std::make_shared<NiceMock<DMockYearView>>();
    // NOLINTNEXTLINE
    std::shared_ptr<NiceMock<DMockRepo>> mockRepo = std::make_shared<NiceMock<DMockRepo>>();
    std::shared_ptr<MockEditor> mockEditor = std::make_shared<MockEditor>(); // NOLINT

    /**
     * @brief Create a caps_log::App object with the mock objects and selected year.
     */
    auto makeCapsLog() {
        AppConfig c;
        c.currentYear = day1.year();
        c.skipFirstLine = true;
        return App{mockView, mockRepo, mockEditor, std::nullopt, std::move(c)};
    }

    void verifyTagMenuItems(const std::vector<std::string> &expected) {
        const auto &tagMenuItems = mockView->getDummyView().m_tagMenuItems;
        EXPECT_EQ(tagMenuItems.getDisplayTexts(), expected);
    }

    void verifySectionMenuItems(const std::vector<std::string> &expected) {
        const auto &sectionMenuItems = mockView->getDummyView().m_sectionMenuItems;
        EXPECT_EQ(sectionMenuItems.getDisplayTexts(), expected);
    }

  public:
    ControllerTest() { mockView->getDummyView().m_focusedDate = day1; }
};

TEST_F(ControllerTest, EscQuits) {
    auto capsLog = makeCapsLog();

    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        EXPECT_CALL(*mockView, stop()).Times(1);
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::Escape.input()}});
    });
    capsLog.run();
    // clear expectation for stop so to not trigger the failure during destruction of the
    // caps log instance
    ::testing::Mock::VerifyAndClearExpectations(mockView.get());
}

TEST_F(ControllerTest, SpecialCharsDontQuit) {
    auto capsLog = makeCapsLog();

    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        EXPECT_CALL(*mockView, stop).Times(0);
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::ArrowDown.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::ArrowUp.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::ArrowLeft.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::ArrowRight.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::Tab.input()}});
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{ftxui::Event::TabReverse.input()}});
    });

    capsLog.run();
    // clear expectation for stop so to not trigger the failure during destruction of the
    // caps log instance
    ::testing::Mock::VerifyAndClearExpectations(mockView.get());
}

TEST_F(ControllerTest, RemoveLog_PromptsThenUpdatesSectionsTagsAndMaps) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# Dummy Section\n* Dummy Tag"});
    auto capsLog = makeCapsLog();

    verifyTagMenuItems({"<select none>", "(1) - dummy tag"});
    verifySectionMenuItems({"<select none>", "(1) - dummy section"});

    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        EXPECT_CALL(*mockView, prompt(_, _));
        ON_CALL(*mockView, prompt).WillByDefault([&](const auto &, const auto &callback) {
            // trigger callback as if user clicked 'yes'
            callback();
        });
        capsLog.handleInputEvent(UIEvent{UnhandledRootEvent{"d"}});
        // assert that it has indeed been removed
        EXPECT_FALSE(mockRepo->read(day1));
        // and that the view things have been updated
        verifyTagMenuItems({"<select none>"});
        verifySectionMenuItems({"<select none>"});
    });
    capsLog.run();
}

TEST_F(ControllerTest, OnFocusedDateChange_UpdatePreviewString) {
    auto dummyLog1 = LogFile{day1, "dummy content 1"};
    auto dummyLog2 = LogFile{day2, "dummy content 2"};
    mockRepo->write(dummyLog1);
    mockRepo->write(dummyLog2);
    auto capsLog = makeCapsLog();
    ASSERT_EQ(mockView->getDummyView().m_previewString, dummyLog1.getContent());
    ASSERT_EQ(mockView->getDummyView().m_focusedDate, dummyLog1.getDate());

    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        mockView->getDummyView().m_focusedDate = dummyLog2.getDate();
        capsLog.handleInputEvent(UIEvent{FocusedDateChange{}});
        ASSERT_EQ(mockView->getDummyView().m_previewString, dummyLog2.getContent());
    });

    capsLog.run();
}

TEST_F(ControllerTest, OnSelectedMenuItemChange_UpdateHighlightMap) {
    auto dummyLog1 = LogFile{day1, "\n# sectone \n* tagone"};
    auto dummyLog2 = LogFile{day2, "\n# secttwo \n* tagtwo"};
    mockRepo->getDummyRepo().write(dummyLog1);
    mockRepo->getDummyRepo().write(dummyLog2);
    auto capsLog = makeCapsLog();

    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&]() { // NOLINT
        // Tags and sections are passed from the view as an index in the section/tagMenuItem
        // vector 0 = '------' aka nothing selected

        auto &dummyView = mockView->getDummyView();

        dummyView.m_selectedTag = "tagone";
        capsLog.handleInputEvent(UIEvent{FocusedTagChange{}});

        using date::monthDay;
        // Tags
        ASSERT_NE(dummyView.m_highlightedDates, nullptr);
        ASSERT_TRUE(dummyView.m_highlightedDates->contains(monthDay(dummyLog1.getDate())));
        ASSERT_FALSE(dummyView.m_highlightedDates->contains(monthDay(dummyLog2.getDate())));

        dummyView.m_selectedTag = "tagtwo";
        capsLog.handleInputEvent(UIEvent{FocusedTagChange{}});
        ASSERT_NE(dummyView.m_highlightedDates, nullptr);
        ASSERT_FALSE(dummyView.m_highlightedDates->contains(monthDay(dummyLog1.getDate())));
        ASSERT_TRUE(dummyView.m_highlightedDates->contains(monthDay(dummyLog2.getDate())));

        // Sections
        dummyView.m_selectedSection = "sectone";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        ASSERT_NE(dummyView.m_highlightedDates, nullptr);
        ASSERT_TRUE(dummyView.m_highlightedDates->contains(monthDay(dummyLog1.getDate())));
        ASSERT_FALSE(dummyView.m_highlightedDates->contains(monthDay(dummyLog2.getDate())));

        dummyView.m_selectedSection = "secttwo";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        ASSERT_NE(dummyView.m_highlightedDates, nullptr);
        ASSERT_FALSE(dummyView.m_highlightedDates->contains(monthDay(dummyLog1.getDate())));
        ASSERT_TRUE(dummyView.m_highlightedDates->contains(monthDay(dummyLog2.getDate())));
    });
    capsLog.run();
}

TEST_F(ControllerTest, AddLog_UpdatesSectionsTagsAndMaps) {
    auto capsLog = makeCapsLog();
    verifyTagMenuItems({"<select none>"});
    verifySectionMenuItems({"<select none>"});

    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        // expect editor to open
        EXPECT_CALL(*mockEditor, openEditor(_)).WillRepeatedly([&](const auto &) {
            auto log = LogFile{day1, "\n# section title \nsome dummy content\n * tag title"};
            mockRepo->getDummyRepo().write(log);
        });

        // on callendar button click
        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});

        // expect the initialy set availability map pointer to still be valid
        verifyTagMenuItems({"<select none>", "(1) - tag title"});
        verifySectionMenuItems({"<select none>", "(1) - section title"});
    });

    capsLog.run();
}

TEST_F(ControllerTest, AddLog_WritesABaslineTemplateForEmptyLog) {
    auto capsLog = makeCapsLog();
    EXPECT_FALSE(mockView->getDummyView().m_datesWithLogs->contains(date::monthDay(day1)));

    ON_CALL(*mockView, run()).WillByDefault([&] {
        EXPECT_CALL(*mockEditor, openEditor(_)).WillRepeatedly([&](const auto &) {
            const auto baseLog = mockRepo->getDummyRepo().read(day1);
            ASSERT_TRUE(baseLog);
            EXPECT_EQ(baseLog->getContent(), date::formatToString(day1, kLogBaseTemplate));
        });
        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});
    });

    capsLog.run();
}

TEST_F(ControllerTest, AddLog_AddedEmptyLogGetsRemoved) {
    auto capsLog = makeCapsLog();
    EXPECT_FALSE(mockView->getDummyView().m_datesWithLogs->contains(date::monthDay(day1)));

    ON_CALL(*mockView, run()).WillByDefault([&] {
        EXPECT_CALL(*mockEditor, openEditor(_))
            .WillOnce([&](const auto &) {
                // Assert only base template has been written
                auto baseLog = mockRepo->getDummyRepo().read(day1);
                ASSERT_TRUE(baseLog);
                EXPECT_EQ(baseLog->getContent(), date::formatToString(day1, kLogBaseTemplate));
            })
            .WillOnce([&](const auto &) {
                // Write an empty log
                mockRepo->getDummyRepo().write({day1, ""});
            });

        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});
        EXPECT_FALSE(mockRepo->getDummyRepo().read(day1));

        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});
        EXPECT_FALSE(mockRepo->getDummyRepo().read(day1));
    });
    capsLog.run();
}

TEST_F(ControllerTest, AddLog_ConfigSkipsFirstSection) {
    {
        auto capsLog = makeCapsLog();
        verifySectionMenuItems({"<select none>"});
    }
    mockRepo->write(LogFile{day1, "# Dummy section"});
    {
        AppConfig c;
        c.skipFirstLine = false;
        c.currentYear = day1.year();
        auto capsLog2 = App{mockView, mockRepo, mockEditor, std::nullopt, std::move(c)};

        // +1 for '-----' aka no section
        verifySectionMenuItems({"<select none>", "(1) - dummy section"});
    }
}

TEST_F(ControllerTest, AddLog_UpdatesSectionsTagsAndMapsAfterRemove) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# Dummy Section\n* Dummy Tag"});
    auto capsLog = makeCapsLog();
    verifyTagMenuItems({"<select none>", "(1) - dummy tag"});
    verifySectionMenuItems({"<select none>", "(1) - dummy section"});

    EXPECT_TRUE(mockView->getDummyView().m_datesWithLogs->contains(date::monthDay(day1)));

    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        // expect editor to open
        EXPECT_CALL(*mockEditor, openEditor(_)).WillRepeatedly([&](const auto &) {
            const auto log = LogFile{day1, "\n* tag title"};
            mockRepo->getDummyRepo().write(log);
        });

        mockView->getDummyView().m_selectedSection = "dummy section";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});

        EXPECT_EQ(mockView->getDummyView().m_highlightedDates, nullptr);
        EXPECT_TRUE(mockView->getDummyView().m_datesWithLogs->contains(date::monthDay(day1)));
        verifyTagMenuItems({"<select none>", "(1) - tag title"});
        verifySectionMenuItems({"<select none>", "(1) - <root section>"});
    });

    capsLog.run();
}

TEST_F(ControllerTest, TagsPerSection_NoSectionMeansAllTagsListed) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# section 1 \n* tag 1"});
    mockRepo->write(LogFile{day2, "# DummyContent \n# section 2 \n* tag 2"});
    auto capsLog = makeCapsLog();
    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        verifyTagMenuItems({"<select none>", "(1) - tag 1", "(1) - tag 2"});
        verifySectionMenuItems({"<select none>", "(1) - section 1", "(1) - section 2"});
    });
    capsLog.run();
}

TEST_F(ControllerTest, TagsPerSection_SelectingASectionShowsOnlyTagsAfterThatSection) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# section 1 \n* tag 1"});
    mockRepo->write(LogFile{day2, "# DummyContent \n# section 2 \n* tag 2"});
    auto capsLog = makeCapsLog();
    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        mockView->getDummyView().m_selectedSection = "section 1";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        verifyTagMenuItems({"<select none>", "(1) - tag 1"});
        EXPECT_EQ(*(mockView->getDummyView().m_highlightedDates), std::set{date::monthDay(day1)});
    });
    capsLog.run();
}

TEST_F(ControllerTest, TagsPerSection_RootSectionShownWhenTagsInRootSectionArePresent) {
    mockRepo->write(LogFile{day1, "# DummyContent \n* tag 1"});
    auto capsLog = makeCapsLog();
    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        verifySectionMenuItems({"<select none>", "(1) - <root section>"});
    });
    capsLog.run();
}

TEST_F(ControllerTest, TagsPerSection_RootSectionNotShownWhenTagsInRootSectionNotPresent) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# section\n* tag 1"});
    auto capsLog = makeCapsLog();
    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        verifySectionMenuItems({"<select none>", "(1) - section"});
    });
    capsLog.run();
}

TEST_F(ControllerTest,
       TagsPerSection_SelectingATagPerSectionHighlightsOnlyTheDatesWhereTagIsUnderThatSection) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# section 1 \n* tag"});
    mockRepo->write(LogFile{day2, "# DummyContent \n# section 1 \nno tag"});
    mockRepo->write(LogFile{day3, "# DummyContent \n# section 2 \n* tag"});
    auto capsLog = makeCapsLog();
    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        // before selection
        verifyTagMenuItems({"<select none>", "(2) - tag"});
        verifySectionMenuItems({"<select none>", "(2) - section 1", "(1) - section 2"});
        EXPECT_EQ(mockView->getDummyView().m_highlightedDates, nullptr);
        EXPECT_EQ(mockView->getDummyView().m_datesWithLogs->size(), 3);

        // after selection
        mockView->getDummyView().m_selectedSection = "section 1";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        verifyTagMenuItems({"<select none>", "(1) - tag"});
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "<select none>");
        const auto expecedDates = std::set{date::monthDay(day1), date::monthDay(day2)};
        EXPECT_EQ(*(mockView->getDummyView().m_highlightedDates), expecedDates);

        // after selection of tag
        mockView->getDummyView().m_selectedTag = "tag";
        capsLog.handleInputEvent(UIEvent{FocusedTagChange{}});
        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "section 1");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "tag");
        EXPECT_EQ(*(mockView->getDummyView().m_highlightedDates), std::set{date::monthDay(day1)});
    });
    capsLog.run();
}

TEST_F(ControllerTest, SectionWithNoTagIsListedInMenuItems) {
    mockRepo->write(LogFile{day2, "# DummyContent \n# section 1 \nno tag"});
    auto capsLog = makeCapsLog();
    EXPECT_CALL(*mockView, run());
    ON_CALL(*mockView, run()).WillByDefault([&] {
        // before selection
        verifySectionMenuItems({"<select none>", "(1) - section 1"});
    });
    capsLog.run();
}

TEST_F(
    ControllerTest,
    TagsPerSection_WhenSectionAndTagSelected_WhenLogEdited_WhenTagIsNoLongerPresent_NoneTagUnderSameSectionSelected) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# section 1 \n* target tag"});
    mockRepo->write(LogFile{day2, "# DummyContent \n# section 1 \nno tag"});
    auto capsLog = makeCapsLog();

    ON_CALL(*mockView, run()).WillByDefault([&] {
        mockView->getDummyView().m_selectedSection = "section 1";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        mockView->getDummyView().m_selectedTag = "target tag";
        capsLog.handleInputEvent(UIEvent{FocusedTagChange{}});
        EXPECT_CALL(*mockEditor, openEditor(_)).WillRepeatedly([&](const LogFile &log) {
            const auto newLog = LogFile(log.getDate(), "# DummyContent \n# section 1 \nno tag");
            mockRepo->getDummyRepo().write(newLog);
        });

        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "section 1");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "target tag");
        const auto expectedTags = std::vector<std::string>{"<select none>", "(1) - target tag"};
        EXPECT_EQ(mockView->getDummyView().m_tagMenuItems.getDisplayTexts(), expectedTags);
        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});
        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "section 1");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "<select none>");
        const auto expectedTagsAfter = std::vector<std::string>{"<select none>"};
        EXPECT_EQ(mockView->getDummyView().m_tagMenuItems.getDisplayTexts(), expectedTagsAfter);
    });
    capsLog.run();
}

TEST_F(
    ControllerTest,
    TagsPerSection_WhenSectionAndTagSelected_WhenLogEdited_WhenSectionIsNoLongerPresent_TagStillPresent_TagSelected) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# section 1 \n* target tag"});
    mockRepo->write(LogFile{day2, "# DummyContent \n# section 2 \nno tag"});
    auto capsLog = makeCapsLog();
    ON_CALL(*mockView, run()).WillByDefault([&] {
        mockView->getDummyView().m_selectedSection = "section 1";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        mockView->getDummyView().m_selectedTag = "target tag";
        capsLog.handleInputEvent(UIEvent{FocusedTagChange{}});
        EXPECT_CALL(*mockEditor, openEditor(_)).WillRepeatedly([&](const LogFile &log) {
            const auto newLog =
                LogFile(log.getDate(), "# DummyContent \n# section new \n* target tag");
            mockRepo->getDummyRepo().write(newLog);
        });

        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "section 1");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "target tag");
        const auto expectedTags = std::vector<std::string>{"<select none>", "(1) - target tag"};
        EXPECT_EQ(mockView->getDummyView().m_tagMenuItems.getDisplayTexts(), expectedTags);
        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});
        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "<select none>");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "target tag");
    });
    capsLog.run();
}

TEST_F(
    ControllerTest,
    TagsPerSection_WhenSectionAndTagSelected_WhenLogEdited_WhenSectionIsNoLongerPresent_TagNotPresent_TagSelected) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# section 1 \n* target tag"});
    mockRepo->write(LogFile{day2, "# DummyContent \n# section 2 \nno tag"});
    auto capsLog = makeCapsLog();
    ON_CALL(*mockView, run()).WillByDefault([&] {
        mockView->getDummyView().m_selectedSection = "section 1";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        mockView->getDummyView().m_selectedTag = "target tag";
        capsLog.handleInputEvent(UIEvent{FocusedTagChange{}});
        EXPECT_CALL(*mockEditor, openEditor(_)).WillRepeatedly([&](const LogFile &log) {
            const auto newLog =
                LogFile(log.getDate(), "# DummyContent \n# section new \n* target tag 2");
            mockRepo->getDummyRepo().write(newLog);
        });

        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "section 1");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "target tag");
        const auto expectedTags = std::vector<std::string>{"<select none>", "(1) - target tag"};
        EXPECT_EQ(mockView->getDummyView().m_tagMenuItems.getDisplayTexts(), expectedTags);
        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});
        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "<select none>");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "<select none>");
    });
    capsLog.run();
}

TEST_F(
    ControllerTest,
    TagsPerSection_WhenSectionAndTagSelected_WhenLogEdited_WhenSectionPresentButMoved_TagPresent_SameSectionSelected) {
    mockRepo->write(LogFile{day1, "# DummyContent \n# section 1 \n* target tag"});
    mockRepo->write(LogFile{day2, "# DummyContent \n# section 2 \n* tag 2"});
    auto capsLog = makeCapsLog();

    ON_CALL(*mockView, run()).WillByDefault([&] {
        mockView->getDummyView().m_selectedSection = "section 2";
        capsLog.handleInputEvent(UIEvent{FocusedSectionChange{}});
        mockView->getDummyView().m_selectedTag = "tag 2";
        capsLog.handleInputEvent(UIEvent{FocusedTagChange{}});

        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "section 2");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "tag 2");
        verifyTagMenuItems({"<select none>", "(1) - tag 2"});
        verifySectionMenuItems({"<select none>", "(1) - section 1", "(1) - section 2"});

        EXPECT_CALL(*mockEditor, openEditor(_)).WillRepeatedly([&](const LogFile &log) {
            const auto newLog =
                LogFile(log.getDate(), "# DummyContent \n# section 3 \n* target tag 2");
            mockRepo->getDummyRepo().write(newLog);
        });
        capsLog.handleInputEvent(UIEvent{OpenLogFile{day1}});

        EXPECT_EQ(mockView->getDummyView().m_selectedSection, "section 2");
        EXPECT_EQ(mockView->getDummyView().m_selectedTag, "tag 2");
        verifyTagMenuItems({"<select none>", "(1) - tag 2"});
        verifySectionMenuItems({"<select none>", "(1) - section 2", "(1) - section 3"});
    });
    capsLog.run();
}

} // namespace caps_log::test
