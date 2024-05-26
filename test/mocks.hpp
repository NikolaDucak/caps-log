#include "editor/editor_base.hpp"
#include "log/log_repository_base.hpp"
#include "utils/date.hpp"
#include "view/annual_view_base.hpp"
#include "view/calendar_component.hpp"
#include "view/input_handler.hpp"
#include <chrono>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

class SpyAnnualView : public caps_log::view::AnnualViewBase {
  public:
    AnnualViewBase *subject;
    SpyAnnualView(AnnualViewBase *subject) : subject(subject) {
        ON_CALL(*this, setInputHandler).WillByDefault([&](auto handler) {
            subject->setInputHandler(handler);
        });
        ON_CALL(*this, getFocusedDate).WillByDefault([&]() { return subject->getFocusedDate(); });
        ON_CALL(*this, setAvailableLogsMap).WillByDefault([&](auto map) {
            subject->setAvailableLogsMap(map);
        });
        ON_CALL(*this, setHighlightedLogsMap).WillByDefault([&](auto map) {
            subject->setHighlightedLogsMap(map);
        });
        ON_CALL(*this, showCalendarForYear).WillByDefault([&](auto year) {
            subject->showCalendarForYear(year);
        });
        ON_CALL(*this, setTagMenuItems).WillByDefault([&](auto items) {
            subject->setTagMenuItems(items);
        });
        ON_CALL(*this, setSectionMenuItems).WillByDefault([&](auto items) {
            subject->setSectionMenuItems(items);
        });
        ON_CALL(*this, withRestoredIO).WillByDefault([&](auto func) { func(); });
        ON_CALL(*this, prompt).WillByDefault([&](auto msg, auto callback) {
            subject->prompt(msg, callback);
        });
        ON_CALL(*this, setPreviewString).WillByDefault([&](auto str) {
            subject->setPreviewString(str);
        });

        ON_CALL(*this, selectedTag).WillByDefault(::testing::ReturnRef(subject->selectedTag()));
        ON_CALL(*this, selectedSection)
            .WillByDefault(::testing::ReturnRef(subject->selectedSection()));
    }

    MOCK_METHOD(void, post, (ftxui::Task), (override));

    MOCK_METHOD(void, promptOk, (std::string message, std::function<void()> callback), (override));
    MOCK_METHOD(void, loadingScreen, (const std::string &str), (override));
    MOCK_METHOD(void, loadingScreenOff, (), (override));

    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, stop, (), (override));

    MOCK_METHOD(void, setInputHandler, (caps_log::view::InputHandlerBase * handler), (override));
    MOCK_METHOD(std::chrono::year_month_day, getFocusedDate, (), (const override));
    MOCK_METHOD(void, showCalendarForYear, (std::chrono::year), (override));
    MOCK_METHOD(void, prompt, (std::string message, std::function<void()> callback), (override));

    MOCK_METHOD(void, setAvailableLogsMap, (const caps_log::utils::date::AnnualMap<bool> *map),
                (override));
    MOCK_METHOD(void, setHighlightedLogsMap, (const caps_log::utils::date::AnnualMap<bool> *map),
                (override));

    MOCK_METHOD(void, setTagMenuItems, (std::vector<std::string> items), (override));
    MOCK_METHOD(void, setSectionMenuItems, (std::vector<std::string> items), (override));

    MOCK_METHOD(void, setPreviewString, (const std::string &string), (override));
    MOCK_METHOD(void, withRestoredIO, (std::function<void()> func), (override));

    MOCK_METHOD(int &, selectedTag, (), (override));
    MOCK_METHOD(int &, selectedSection, (), (override));
};

class DummyYearView : public caps_log::view::AnnualViewBase {
  public:
    caps_log::view::InputHandlerBase *m_inputHandler = nullptr;
    std::chrono::year m_displayedYear;
    int m_selectedTag, m_selectedSection;
    std::chrono::year_month_day m_focusedDate{std::chrono::year{2005}, std::chrono::month{1},
                                              std::chrono::day{1}};
    std::string m_previewString;
    const caps_log::utils::date::AnnualMap<bool> *m_availableLogsMap, *m_highlightedLogsMap;
    std::vector<std::string> m_tagMenuItems, m_sectionMenuItems;

    void run() override {}
    void stop() override {}
    void post(ftxui::Task /*unused*/) override {}

    void promptOk(std::string message, std::function<void()> callback) override {}
    void loadingScreen(const std::string &str) override {}
    void loadingScreenOff() override {}

    void setInputHandler(caps_log::view::InputHandlerBase *handler) override {
        m_inputHandler = handler;
    }
    std::chrono::year_month_day getFocusedDate() const override { return m_focusedDate; }
    void showCalendarForYear(std::chrono::year year) override { m_displayedYear = year; }

    std::function<void()> promptCallback = nullptr;
    void prompt(std::string message, std::function<void()> callback) override {
        promptCallback = callback;
    }

    void setAvailableLogsMap(const caps_log::utils::date::AnnualMap<bool> *map) override {
        m_availableLogsMap = map;
    }
    void setHighlightedLogsMap(const caps_log::utils::date::AnnualMap<bool> *map) override {
        m_highlightedLogsMap = map;
    }

    void setTagMenuItems(std::vector<std::string> items) override { m_tagMenuItems = items; }
    void setSectionMenuItems(std::vector<std::string> items) override {
        m_sectionMenuItems = items;
    }

    void setPreviewString(const std::string &string) override { m_previewString = string; }
    void withRestoredIO(std::function<void()> func) override { func(); }

    int &selectedTag() override { return m_selectedTag; }
    int &selectedSection() override { return m_selectedSection; }
};

class DMockYearView : public caps_log::view::AnnualViewBase {
    DummyYearView view;

  public:
    DMockYearView() {
        ON_CALL(*this, setInputHandler).WillByDefault([&](auto handler) {
            view.setInputHandler(handler);
        });
        ON_CALL(*this, getFocusedDate).WillByDefault([&]() { return view.getFocusedDate(); });
        ON_CALL(*this, setAvailableLogsMap).WillByDefault([&](auto map) {
            view.setAvailableLogsMap(map);
        });
        ON_CALL(*this, setHighlightedLogsMap).WillByDefault([&](auto map) {
            view.setHighlightedLogsMap(map);
        });
        ON_CALL(*this, showCalendarForYear).WillByDefault([&](auto year) {
            view.showCalendarForYear(year);
        });
        ON_CALL(*this, setTagMenuItems).WillByDefault([&](auto items) {
            view.setTagMenuItems(items);
        });
        ON_CALL(*this, setSectionMenuItems).WillByDefault([&](auto items) {
            view.setSectionMenuItems(items);
        });
        ON_CALL(*this, withRestoredIO).WillByDefault([&](auto func) { func(); });
        ON_CALL(*this, prompt).WillByDefault([&](auto msg, auto callback) {
            view.prompt(msg, callback);
        });
        ON_CALL(*this, setPreviewString).WillByDefault([&](auto str) {
            view.setPreviewString(str);
        });

        ON_CALL(*this, selectedTag).WillByDefault(::testing::ReturnRef(view.selectedTag()));
        ON_CALL(*this, selectedSection).WillByDefault(::testing::ReturnRef(view.selectedSection()));
    }

    auto &getDummyView() { return view; }

    MOCK_METHOD(void, post, (ftxui::Task), (override));

    MOCK_METHOD(void, promptOk, (std::string message, std::function<void()> callback), (override));
    MOCK_METHOD(void, loadingScreen, (const std::string &str), (override));
    MOCK_METHOD(void, loadingScreenOff, (), (override));

    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, stop, (), (override));

    MOCK_METHOD(void, setInputHandler, (caps_log::view::InputHandlerBase * handler), (override));
    MOCK_METHOD(std::chrono::year_month_day, getFocusedDate, (), (const override));
    MOCK_METHOD(void, showCalendarForYear, (std::chrono::year), (override));
    MOCK_METHOD(void, prompt, (std::string message, std::function<void()> callback), (override));

    MOCK_METHOD(void, setAvailableLogsMap, (const caps_log::utils::date::AnnualMap<bool> *map),
                (override));
    MOCK_METHOD(void, setHighlightedLogsMap, (const caps_log::utils::date::AnnualMap<bool> *map),
                (override));

    MOCK_METHOD(void, setTagMenuItems, (std::vector<std::string> items), (override));
    MOCK_METHOD(void, setSectionMenuItems, (std::vector<std::string> items), (override));

    MOCK_METHOD(void, setPreviewString, (const std::string &string), (override));
    MOCK_METHOD(void, withRestoredIO, (std::function<void()> func), (override));

    MOCK_METHOD(int &, selectedTag, (), (override));
    MOCK_METHOD(int &, selectedSection, (), (override));
};

class DummyRepository : public caps_log::log::LogRepositoryBase {
    std::map<std::chrono::year_month_day, std::string> m_data;

  public:
    std::optional<caps_log::log::LogFile>
    read(const std::chrono::year_month_day &date) const override {
        if (auto it = m_data.find(date); it != m_data.end()) {
            return caps_log::log::LogFile{it->first, it->second};
        }
        return {};
    }

    void remove(const std::chrono::year_month_day &date) override { m_data.erase(date); }

    void write(const caps_log::log::LogFile &file) override {
        m_data[file.getDate()] = file.getContent();
    }
};

class DMockRepo : public caps_log::log::LogRepositoryBase {
    DummyRepository m_repo;

  public:
    DMockRepo() {
        ON_CALL(*this, read).WillByDefault([this](const auto &date) { return m_repo.read(date); });
        ON_CALL(*this, write).WillByDefault([this](const auto &log) { return m_repo.write(log); });
        ON_CALL(*this, remove).WillByDefault([this](const auto &date) {
            return m_repo.remove(date);
        });
    }

    auto &getDummyRepo() { return m_repo; }

    MOCK_METHOD(std::optional<caps_log::log::LogFile>, read,
                (const std::chrono::year_month_day &date), (const override));
    MOCK_METHOD(void, remove, (const std::chrono::year_month_day &date), (override));
    MOCK_METHOD(void, write, (const caps_log::log::LogFile &file), (override));
};

class MockEditor : public caps_log::editor::EditorBase {
  public:
    MOCK_METHOD(void, openEditor, (const caps_log::log::LogFile &), (override));
};
