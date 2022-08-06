#include "date/date.hpp"
#include "editor/editor_base.hpp"
#include "model/log_repository_base.hpp"
#include "view/calendar_component.hpp"
#include "view/input_handler.hpp"
#include "view/year_view_base.hpp"
#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

class DummyYearView : public clog::view::YearViewBase {
public:
    clog::view::InputHandlerBase* m_inputHandler = nullptr;
    int m_displayedYear;
    int m_selectedTag, m_selectedSection;
    clog::date::Date m_focusedDate{5,5,2005};
    std::string m_previewString;
    const clog::date::YearMap<bool>* m_availableLogsMap, *m_highlightedLogsMap;
    std::vector<std::string> m_tagMenuItems , m_sectionMenuItems;

    void run() override {}
    void stop() override {}

    void setInputHandler(clog::view::InputHandlerBase* handler) override { m_inputHandler = handler; }
    clog::date::Date getFocusedDate() const override { return m_focusedDate; }
    void showCalendarForYear(unsigned year) override { m_displayedYear = year; }

    std::function<void()> promptCallback = nullptr;
    void prompt(std::string message, std::function<void()> callback) override { promptCallback = callback; }

    void setAvailableLogsMap(const clog::date::YearMap<bool>* map) override { m_availableLogsMap = map; }
    void setHighlightedLogsMap(const clog::date::YearMap<bool>* map) override { m_highlightedLogsMap = map; }

    void setTagMenuItems(std::vector<std::string> items) override { m_tagMenuItems = items; }
    void setSectionMenuItems(std::vector<std::string> items) override { m_sectionMenuItems = items; }

    void setPreviewString(const std::string& string) override { m_previewString = string; }
    void withRestoredIO(std::function<void()> func) override {func();}

    int& selectedTag() override { return m_selectedTag; }
    int& selectedSection() override { return m_selectedSection; }
};

class DMockYearView : public clog::view::YearViewBase {
    DummyYearView view;
public:
    DMockYearView() {
        ON_CALL(*this, setInputHandler)
            .WillByDefault([&](auto h){ view.setInputHandler(h); });
        ON_CALL(*this, getFocusedDate)
            .WillByDefault([&](){ return view.getFocusedDate(); });
        ON_CALL(*this, setAvailableLogsMap)
            .WillByDefault([&](auto m){ view.setAvailableLogsMap(m); });
        ON_CALL(*this, setHighlightedLogsMap)
            .WillByDefault([&](auto m){ view.setHighlightedLogsMap(m); });
        ON_CALL(*this, showCalendarForYear)
            .WillByDefault([&](auto y){ view.showCalendarForYear(y); });
        ON_CALL(*this, setTagMenuItems)
            .WillByDefault([&](auto m){ view.setTagMenuItems(m); });
        ON_CALL(*this, setSectionMenuItems)
            .WillByDefault([&](auto m){ view.setSectionMenuItems(m); });
        ON_CALL(*this, withRestoredIO)
            .WillByDefault([&](auto func){ func(); });
        ON_CALL(*this, prompt)
            .WillByDefault([&](auto msg, auto cb){ view.prompt(msg, cb); });
        ON_CALL(*this, setPreviewString)
            .WillByDefault([&](auto str){ view.setPreviewString(str); });

        ON_CALL(*this, selectedTag)
            .WillByDefault(::testing::ReturnRef(view.selectedTag()));
        ON_CALL(*this, selectedSection)
            .WillByDefault(::testing::ReturnRef(view.selectedSection()));
    }

    auto& getDummyView() { return view; }

    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, stop, (), (override));

    MOCK_METHOD(void, setInputHandler , (clog::view::InputHandlerBase* handler), (override));
    MOCK_METHOD(clog::date::Date, getFocusedDate, (), (const override));
    MOCK_METHOD(void, showCalendarForYear, (unsigned year), (override));
    MOCK_METHOD(void, prompt, (std::string message, std::function<void()> callback), (override));

    MOCK_METHOD(void, setAvailableLogsMap, (const clog::date::YearMap<bool>* map), (override));
    MOCK_METHOD(void, setHighlightedLogsMap, (const clog::date::YearMap<bool>* map), (override));

    MOCK_METHOD(void, setTagMenuItems, (std::vector<std::string> items), (override));
    MOCK_METHOD(void, setSectionMenuItems, (std::vector<std::string> items), (override));

    MOCK_METHOD(void, setPreviewString, (const std::string& string), (override));
    MOCK_METHOD(void, withRestoredIO, (std::function<void()> func), (override));

    MOCK_METHOD(int&, selectedTag, (), (override));
    MOCK_METHOD(int&, selectedSection, (), (override));
};

class DummyRepository : public clog::model::LogRepositoryBase {
    std::map<clog::date::Date, std::string> m_data;
public:
    std::optional<clog::model::LogFile> read (const clog::date::Date& date) const override {
        if (auto it = m_data.find(date); it != m_data.end()) {
            return clog::model::LogFile{it->first, it->second};
        }
        return {};
    }

    void remove (const clog::date::Date& date) override {
        if (auto it = m_data.find(date); it != m_data.end()) {
            m_data.erase(it);
        }
    }

    void write (const clog::model::LogFile& file) override {
        m_data[file.getDate()] = file.getContent();
    }
};

class DMockRepo : public clog::model::LogRepositoryBase {
    DummyRepository m_repo;
public:

    DMockRepo() {
        ON_CALL(*this, read)
            .WillByDefault([this](const auto& date) { return m_repo.read(date); });
        ON_CALL(*this, write)
            .WillByDefault([this](const auto& log) { return m_repo.write(log); });
        ON_CALL(*this, remove)
            .WillByDefault([this](const auto& date) { return m_repo.remove(date); });
    }

    auto& getDummyRepo() { return m_repo; }

    MOCK_METHOD(std::optional<clog::model::LogFile>, read, (const clog::date::Date& date), (const override));
    MOCK_METHOD(void, remove, (const clog::date::Date& date), (override));
    MOCK_METHOD(void, write, (const clog::model::LogFile& file), (override));
};

class MockEditor : public clog::editor::EditorBase {
public:
    MOCK_METHOD(void, openEditor, (const clog::model::LogFile&), (override));
};
