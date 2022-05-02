#include "app.hpp"
#include "model/DefaultLogRepository.hpp"
#include "model/LogRepositoryBase.hpp"
#include "view/input_handler.hpp"
#include "view/yearly_view.hpp"
#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <fstream>

using namespace clog;
using namespace testing;

class MockYearView : public view::YearViewBase {
public:
    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, setInputHandler , (InputHandlerBase* handler), (override));
    MOCK_METHOD(model::Date, getFocusedDate, (), (const override));
    MOCK_METHOD(void, showCalendarForYear, (unsigned year), (override));
    MOCK_METHOD(void, prompt, (std::string message, std::function<void()> callback), (override));
    MOCK_METHOD(void, setAvailableLogsMap, (const model::YearMap<bool>* map), (override));
    MOCK_METHOD(void, setHighlightedLogsMap, (const model::YearMap<bool>* map), (override));
    MOCK_METHOD(void, setTagMenuItems, (std::vector<std::string> items), (override));
    MOCK_METHOD(void, setSectionMenuItems, (std::vector<std::string> items), (override));
    MOCK_METHOD(void, setPreviewString, (const std::string& string), (override));
    MOCK_METHOD(void, withRestoredIO, (std::function<void()> func), (override));
    MOCK_METHOD(int&, selectedTag, (), (override));
    MOCK_METHOD(int&, selectedSection, (), (override));
};

class MockRepo : public model::LogRepositoryBase {
public:
    MOCK_METHOD(YearLogEntryData , collectDataForYear, (unsigned year), (override));
    MOCK_METHOD(void, injectDataForDate, (YearLogEntryData&, const Date&), (override));
    MOCK_METHOD(std::optional<LogFile>, readLogFile,(const Date& date), (override));
    MOCK_METHOD(LogFile, readOrMakeLogFile, (const Date& date), (override));
    MOCK_METHOD(void, removeLog, (const Date& date), (override));
    MOCK_METHOD(std::string, path, (const Date& date), (override));
};

TEST(ContollerTest, EscQuits) {
    const auto mock_view = std::make_shared<testing::NiceMock<MockYearView>>();
    const auto mock_repo = std::make_shared<testing::NiceMock<MockRepo>>();
    auto clog = clog::App{mock_view, mock_repo};

    EXPECT_CALL(*mock_view, run());
    ON_CALL(*mock_view, run()).WillByDefault([&] {
        EXPECT_CALL(*mock_view, stop());
        clog.handleInputEvent(UIEvent{UIEvent::ROOT_EVENT, '\x1B'});
    });

    clog.run();
}

TEST(ContollerTest, RemoveLog) {
    const auto mock_view = std::make_shared<testing::NiceMock<MockYearView>>();
    const auto mock_repo = std::make_shared<testing::NiceMock<MockRepo>>();
    const auto selectedDate = model::Date{25,5,2005};
    auto map = YearMap<bool>{};
    map.set(selectedDate, true);
    const auto data = YearLogEntryData { .logAvailabilityMap = map };

    EXPECT_CALL(*mock_repo, collectDataForYear(model::Date::getToday().year))
        .WillOnce(Return(data));
    auto clog = clog::App{mock_view, mock_repo};

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
    auto clog = clog::App{mock_view, mock_repo};

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


