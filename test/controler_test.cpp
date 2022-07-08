#include "app.hpp"
#include "mocks.hpp"

#include "model/default_log_repository.hpp"
#include <fstream>

using namespace clog;
using namespace testing;

TEST(ContollerTest, EscQuits) {
    const auto mock_view   = std::make_shared<testing::NiceMock<MockYearView>>();
    const auto mock_repo   = std::make_shared<testing::NiceMock<MockRepo>>();
    const auto mock_editor = std::make_shared<MockEditor>();
    auto clog = clog::App{mock_view, mock_repo, mock_editor};

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_view, stop());
        clog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, '\x1B'});
    });

    clog.run();
}

TEST(ContollerTest, RemoveLog) {
    const auto mock_view   = std::make_shared<testing::NiceMock<MockYearView>>();
    const auto mock_repo   = std::make_shared<testing::NiceMock<MockRepo>>();
    const auto mock_editor = std::make_shared<MockEditor>();
    const auto selectedDate = model::Date{25,5,2005};
    auto map = YearMap<bool>{};
    map.set(selectedDate, true);
    const auto data = YearLogEntryData { .logAvailabilityMap = map };

    EXPECT_CALL(*mock_repo, collectDataForYear(model::Date::getToday().year))
        .WillOnce(Return(data));
    auto clog = clog::App{mock_view, mock_repo, mock_editor};

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_view, getFocusedDate()).WillOnce(testing::Return(selectedDate));
        EXPECT_CALL(*mock_view, prompt(_, _)).WillOnce(InvokeArgument<1>());
        EXPECT_CALL(*mock_repo, removeLog(selectedDate));
        EXPECT_CALL(*mock_view, setPreviewString(""));
        // TODO: assert that it does set the updated values
        EXPECT_CALL(*mock_view, setSectionMenuItems(_));
        EXPECT_CALL(*mock_view, setTagMenuItems(_));
        EXPECT_CALL(*mock_view, setAvailableLogsMap(_));
        clog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, 'd'});
    });
    clog.run();
}


TEST(ContollerTest, AddLog) {
    const auto mock_view = std::make_shared<testing::NiceMock<MockYearView>>();
    const auto mock_repo = std::make_shared<testing::NiceMock<MockRepo>>();
    const auto mock_editor = std::make_shared<MockEditor>();
    auto clog = clog::App{mock_view, mock_repo, mock_editor};

    ON_CALL(*mock_view, run()).WillByDefault([&] {
        clog.handleInputEvent(UIEvent{UIEvent::CALENDAR_BUTTON_CLICK, '\x1B'});
        // EXPECT_CALL(*mock_editor, openEditor(_,_);
        // TODO: assert that it does set the updated values
        EXPECT_CALL(*mock_view, setSectionMenuItems(_));
        EXPECT_CALL(*mock_view, setTagMenuItems(_));
        EXPECT_CALL(*mock_view, setAvailableLogsMap(_));
    });

    clog.run();
}

// TODO: move to another file -----------------------------------------

static const std::string TEST_LOG_DIRECTORY
    {std::filesystem::temp_directory_path().string() + "/clog_test_dir"};

TEST(DefaultLogRepositoryTest, RemoveLog) {
    const auto selectedDate = model::Date{25,5,2005};
    auto repo = DefaultLogRepository(TEST_LOG_DIRECTORY);

    auto l = repo.readOrMakeLogFile(selectedDate);
    {
        std::ofstream of(repo.path(selectedDate));
        of << "aaa";
    }
    ASSERT_TRUE(repo.collectDataForYear(2005).logAvailabilityMap.get(selectedDate));

    repo.removeLog(selectedDate);
    ASSERT_FALSE(repo.collectDataForYear(2005).logAvailabilityMap.get(selectedDate));
}


