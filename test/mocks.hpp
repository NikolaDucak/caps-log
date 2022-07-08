#include "editor_base.hpp"
#include "model/log_repository_base.hpp"
#include "view/input_handler.hpp"
#include "view/year_view_base.hpp"
#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockYearView : public clog::view::YearViewBase {
public:
    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, setInputHandler , (clog::view::InputHandlerBase* handler), (override));
    MOCK_METHOD(clog::model::Date, getFocusedDate, (), (const override));
    MOCK_METHOD(void, showCalendarForYear, (unsigned year), (override));
    MOCK_METHOD(void, prompt, (std::string message, std::function<void()> callback), (override));
    MOCK_METHOD(void, setAvailableLogsMap, (const clog::model::YearMap<bool>* map), (override));
    MOCK_METHOD(void, setHighlightedLogsMap, (const clog::model::YearMap<bool>* map), (override));
    MOCK_METHOD(void, setTagMenuItems, (std::vector<std::string> items), (override));
    MOCK_METHOD(void, setSectionMenuItems, (std::vector<std::string> items), (override));
    MOCK_METHOD(void, setPreviewString, (const std::string& string), (override));
    MOCK_METHOD(void, withRestoredIO, (std::function<void()> func), (override));
    MOCK_METHOD(int&, selectedTag, (), (override));
    MOCK_METHOD(int&, selectedSection, (), (override));
};

class MockRepo : public clog::model::LogRepositoryBase {
public:
    MOCK_METHOD(clog::model::YearLogEntryData , collectDataForYear, (unsigned year), (override));
    MOCK_METHOD(void, injectDataForDate, (clog::model::YearLogEntryData&, const clog::model::Date&), (override));
    MOCK_METHOD(std::optional<clog::model::LogFile>, readLogFile,(const clog::model::Date& date), (override));
    MOCK_METHOD(clog::model::LogFile, readOrMakeLogFile, (const clog::model::Date& date), (override));
    MOCK_METHOD(void, removeLog, (const clog::model::Date& date), (override));
    MOCK_METHOD(std::string, path, (const clog::model::Date& date), (override));
};

class MockEditor : public clog::EditorBase {
public:
    MOCK_METHOD(void, openEditor, (const std::string& path), (override));
};
