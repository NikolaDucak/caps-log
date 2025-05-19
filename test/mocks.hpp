#include "editor/editor_base.hpp"
#include "log/log_repository_base.hpp"
#include "utils/date.hpp"
#include "view/annual_view_base.hpp"
#include "view/input_handler.hpp"
#include <chrono>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utility>

class DummyYearView : public caps_log::view::AnnualViewBase {
  public:
    caps_log::view::InputHandlerBase *m_inputHandler = nullptr;
    std::chrono::year m_displayedYear{};
    std::chrono::year_month_day m_focusedDate{std::chrono::year{2005}, std::chrono::month{1},
                                              std::chrono::day{1}};
    std::string m_previewString, m_previewTitle;
    const caps_log::utils::date::Dates *m_datesWithLogs{}, *m_highlightedDates{};
    caps_log::view::MenuItems m_tagMenuItems, m_sectionMenuItems;
    std::string m_selectedTag, m_selectedSection;

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

    void setDatesWithLogs(const caps_log::utils::date::Dates *map) override {
        m_datesWithLogs = map;
    }
    void setHighlightedDates(const caps_log::utils::date::Dates *map) override {
        m_highlightedDates = map;
    }

    void setPreviewString(const std::string &title, const std::string &string) override {
        m_previewString = string;
        m_previewTitle = title;
    }
    void withRestoredIO(std::function<void()> func) override { func(); }

    caps_log::view::MenuItems &tagMenuItems() override { return m_tagMenuItems; }
    caps_log::view::MenuItems &sectionMenuItems() override { return m_sectionMenuItems; }
    const std::string &getSelectedTag() const override { return m_selectedTag; }
    const std::string &getSelectedSection() const override { return m_selectedSection; }
    void setSelectedTag(std::string tag) override { m_selectedTag = tag; }
    void setSelectedSection(std::string section) override { m_selectedSection = section; }
};

class DMockYearView : public caps_log::view::AnnualViewBase {
    DummyYearView m_view;

  public:
    DMockYearView() {
        ON_CALL(*this, setInputHandler).WillByDefault([&](auto handler) {
            m_view.setInputHandler(handler);
        });
        ON_CALL(*this, getFocusedDate).WillByDefault([&]() { return m_view.getFocusedDate(); });
        ON_CALL(*this, setDatesWithLogs).WillByDefault([&](auto map) {
            m_view.setDatesWithLogs(map);
        });
        ON_CALL(*this, setHighlightedDates).WillByDefault([&](auto map) {
            m_view.setHighlightedDates(map);
        });
        ON_CALL(*this, showCalendarForYear).WillByDefault([&](auto year) {
            m_view.showCalendarForYear(year);
        });
        ON_CALL(*this, withRestoredIO).WillByDefault([&](const auto &func) { func(); });
        ON_CALL(*this, prompt).WillByDefault([&](auto msg, auto callback) {
            m_view.prompt(std::move(msg), std::move(callback));
        });
        ON_CALL(*this, setPreviewString).WillByDefault([&](const auto &title, const auto &str) {
            m_view.setPreviewString(title, str);
        });

        ON_CALL(*this, tagMenuItems).WillByDefault([&]() -> auto & {
            return m_view.tagMenuItems();
        });
        ON_CALL(*this, sectionMenuItems).WillByDefault([&]() -> auto & {
            return m_view.sectionMenuItems();
        });
        ON_CALL(*this, getSelectedTag).WillByDefault([&]() -> auto & {
            return m_view.getSelectedTag();
        });
        ON_CALL(*this, getSelectedSection).WillByDefault([&]() -> auto & {
            return m_view.getSelectedSection();
        });
        ON_CALL(*this, setSelectedTag).WillByDefault([&](auto tag) {
            m_view.setSelectedTag(std::move(tag));
        });
        ON_CALL(*this, setSelectedSection).WillByDefault([&](auto section) {
            m_view.setSelectedSection(std::move(section));
        });
    }

    auto &getDummyView() { return m_view; }

    MOCK_METHOD(void, post, (ftxui::Task), (override));

    MOCK_METHOD(void, promptOk, (std::string message, std::function<void()> callback), (override));
    MOCK_METHOD(void, loadingScreen, (const std::string &str), (override));
    MOCK_METHOD(void, loadingScreenOff, (), (override));

    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, stop, (), (override));

    MOCK_METHOD(void, setInputHandler, (caps_log::view::InputHandlerBase * handler), (override));
    MOCK_METHOD(std::chrono::year_month_day, getFocusedDate, (), (const, override));
    MOCK_METHOD(void, showCalendarForYear, (std::chrono::year), (override));
    MOCK_METHOD(void, prompt, (std::string message, std::function<void()> callback), (override));

    MOCK_METHOD(void, setDatesWithLogs, (const caps_log::utils::date::Dates *map), (override));
    MOCK_METHOD(void, setHighlightedDates, (const caps_log::utils::date::Dates *map), (override));

    MOCK_METHOD(void, setPreviewString, (const std::string &title, const std::string &string),
                (override));
    MOCK_METHOD(void, withRestoredIO, (std::function<void()> func), (override));

    MOCK_METHOD(caps_log::view::MenuItems &, tagMenuItems, (), (override));
    MOCK_METHOD(caps_log::view::MenuItems &, sectionMenuItems, (), (override));
    MOCK_METHOD(const std::string &, getSelectedTag, (), (const, override));
    MOCK_METHOD(const std::string &, getSelectedSection, (), (const, override));
    MOCK_METHOD(void, setSelectedTag, (std::string tag), (override));
    MOCK_METHOD(void, setSelectedSection, (std::string section), (override));
};

class DummyRepository : public caps_log::log::LogRepositoryBase {
    std::map<std::chrono::year_month_day, std::string> m_data;

  public:
    std::optional<caps_log::log::LogFile>
    read(const std::chrono::year_month_day &date) const override {
        if (auto it = m_data.find(date); it != m_data.end()) {
            return caps_log::log::LogFile{it->first, it->second};
        }
        return std::nullopt;
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
                (const std::chrono::year_month_day &date), (const, override));
    MOCK_METHOD(void, remove, (const std::chrono::year_month_day &date), (override));
    MOCK_METHOD(void, write, (const caps_log::log::LogFile &file), (override));
};

class MockEditor : public caps_log::editor::EditorBase {
  public:
    MOCK_METHOD(void, openEditor, (const caps_log::log::LogFile &), (override));
};
