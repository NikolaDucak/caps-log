#include "editor/editor_base.hpp"
#include "log/log_repository_base.hpp"
#include "utils/date.hpp"
#include "view/annual_view_layout_base.hpp"
#include "view/input_handler.hpp"
#include "view/view.hpp"
#include "view/view_layout_base.hpp"
#include <chrono>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utility>

class DummyAnnualViewLayout : public caps_log::view::AnnualViewLayoutBase {
  public:
    caps_log::view::InputHandlerBase *m_inputHandler = nullptr;
    std::chrono::year m_displayedYear{};
    std::chrono::year_month_day m_focusedDate{std::chrono::year{2005}, std::chrono::month{1},
                                              std::chrono::day{1}};
    std::string m_previewString, m_previewTitle;
    const caps_log::utils::date::Dates *m_datesWithLogs{}, *m_highlightedDates{};
    caps_log::view::MenuItems m_tagMenuItems, m_sectionMenuItems;
    std::string m_selectedTag, m_selectedSection;

    [[nodiscard]] std::chrono::year_month_day getFocusedDate() const override {
        return m_focusedDate;
    }
    void showCalendarForYear(std::chrono::year year) override { m_displayedYear = year; }

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

    void setEventDates(const caps_log::view::CalendarEvents *events) override {
        // Not implemented in dummy
    }

    caps_log::view::MenuItems &tagMenuItems() override { return m_tagMenuItems; }
    caps_log::view::MenuItems &sectionMenuItems() override { return m_sectionMenuItems; }
    [[nodiscard]] const std::string &getSelectedTag() const override { return m_selectedTag; }
    [[nodiscard]] const std::string &getSelectedSection() const override {
        return m_selectedSection;
    }
    void setSelectedTag(std::string tag) override { m_selectedTag = tag; }
    void setSelectedSection(std::string section) override { m_selectedSection = section; }

    ftxui::Component getComponent() override { return nullptr; };
};

class DummyScratchpadViewLayout : public caps_log::view::ScratchpadViewLayoutBase {
  public:
    std::vector<caps_log::view::ScratchpadData> m_scratchpads;
    void
    setScratchpads(const std::vector<caps_log::view::ScratchpadData> &scratchpadData) override {
        m_scratchpads = scratchpadData;
    }
    ftxui::Component getComponent() override { return nullptr; }
};

class DMockAnnualViewLayout : public caps_log::view::AnnualViewLayoutBase {
    DummyAnnualViewLayout m_view;

  public:
    DMockAnnualViewLayout() {
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
        ON_CALL(*this, getComponent).WillByDefault([&]() { return m_view.getComponent(); });
    }

    auto &getDummyView() { return m_view; }

    MOCK_METHOD(std::chrono::year_month_day, getFocusedDate, (), (const, override));
    MOCK_METHOD(void, showCalendarForYear, (std::chrono::year), (override));

    MOCK_METHOD(void, setDatesWithLogs, (const caps_log::utils::date::Dates *map), (override));
    MOCK_METHOD(void, setHighlightedDates, (const caps_log::utils::date::Dates *map), (override));

    MOCK_METHOD(void, setPreviewString, (const std::string &title, const std::string &string),
                (override));

    MOCK_METHOD(caps_log::view::MenuItems &, tagMenuItems, (), (override));
    MOCK_METHOD(caps_log::view::MenuItems &, sectionMenuItems, (), (override));
    MOCK_METHOD(const std::string &, getSelectedTag, (), (const, override));
    MOCK_METHOD(const std::string &, getSelectedSection, (), (const, override));
    MOCK_METHOD(void, setSelectedTag, (std::string tag), (override));
    MOCK_METHOD(void, setSelectedSection, (std::string section), (override));
    MOCK_METHOD(ftxui::Component, getComponent, (), (override));
};

class DummyPopUpView : public caps_log::view::PopUpViewBase {
  public:
    PopUpType m_currentPopUp = caps_log::view::PopUpViewBase::None{};

    [[nodiscard]] PopUpType getCurrentPopUp() const { return m_currentPopUp; }
    void show(const PopUpType &popUp) override {
        // Dummy implementation, does nothing
    }
};

class DMockPopupView : public caps_log::view::PopUpViewBase {
  public:
    DummyPopUpView m_view;

    DMockPopupView() {
        ON_CALL(*this, show).WillByDefault([&](const PopUpType &popUp) { m_view.show(popUp); });
    }

    MOCK_METHOD(void, show, (const PopUpType &popUp), (override));

    DummyPopUpView &getDummyView() { return m_view; }
};

class DMockScratchpadViewLayout : public caps_log::view::ScratchpadViewLayoutBase {
  public:
    DummyScratchpadViewLayout m_view;

    DMockScratchpadViewLayout() {
        ON_CALL(*this, setScratchpads).WillByDefault([&](const auto &scratchpadData) {
            m_view.setScratchpads(scratchpadData);
        });
        ON_CALL(*this, getComponent).WillByDefault([&]() { return m_view.getComponent(); });
    }

    MOCK_METHOD(void, setScratchpads, (const std::vector<caps_log::view::ScratchpadData> &),
                (override));
    MOCK_METHOD(ftxui::Component, getComponent, (), (override));

    DummyScratchpadViewLayout &getDummyView() { return m_view; }
};

class DMockPopUpView : public caps_log::view::PopUpViewBase {
  public:
    MOCK_METHOD(void, show, (const PopUpType &popUp), (override));
};

class DMockView : public caps_log::view::ViewBase, public caps_log::view::InputHandlerBase {
  public:
    std::shared_ptr<DMockAnnualViewLayout> m_annualViewLayout =
        std::make_shared<DMockAnnualViewLayout>();
    std::shared_ptr<DMockScratchpadViewLayout> m_scratchpadViewLayout =
        std::make_shared<DMockScratchpadViewLayout>();

    std::shared_ptr<caps_log::view::ViewLayoutBase> m_selectedLayout =
        m_annualViewLayout; // Default to annual view layout

    DMockPopUpView m_popUpView;
    caps_log::view::InputHandlerBase *m_inputHandler = nullptr;

    DMockView() {
        ON_CALL(*this, getAnnualViewLayout).WillByDefault([&]() { return m_annualViewLayout; });
        ON_CALL(*this, getScratchpadViewLayout).WillByDefault([&]() {
            return m_scratchpadViewLayout;
        });
        ON_CALL(*this, getPopUpView).WillByDefault([&]() -> caps_log::view::PopUpViewBase & {
            return m_popUpView;
        });
        ON_CALL(*this, run).WillByDefault([&]() { /* Do nothing */ });
        ON_CALL(*this, stop).WillByDefault([&]() { /* Do nothing */ });
        ON_CALL(*this, post).WillByDefault([&](const ftxui::Task &task) {
            // Do nothing, just a dummy implementation
        });
        ON_CALL(*this, withRestoredIO).WillByDefault([&](const std::function<void()> &func) {
            // Do nothing, just a dummy implementation
            func();
        });
        ON_CALL(*this, setInputHandler)
            .WillByDefault(
                [&](caps_log::view::InputHandlerBase *handler) { m_inputHandler = handler; });
        ON_CALL(*this, handleInputEvent).WillByDefault([&](const caps_log::view::UIEvent &event) {
            if (m_inputHandler) {
                return m_inputHandler->handleInputEvent(event);
            }
            return false; // No input handler set
        });
        ON_CALL(*this, switchLayout).WillByDefault([&]() {
            if (m_selectedLayout == m_annualViewLayout) {
                m_selectedLayout = m_scratchpadViewLayout;
            } else {
                m_selectedLayout = m_annualViewLayout;
            }
        });
        ON_CALL(*this, getPopUpView).WillByDefault([&]() -> caps_log::view::PopUpViewBase & {
            return m_popUpView;
        });
    }

    MOCK_METHOD(std::shared_ptr<caps_log::view::AnnualViewLayoutBase>, getAnnualViewLayout, (),
                (override));
    MOCK_METHOD(std::shared_ptr<caps_log::view::ScratchpadViewLayoutBase>, getScratchpadViewLayout,
                (), (override));
    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, post, (const ftxui::Task &task), (override));
    MOCK_METHOD(void, withRestoredIO, (std::function<void()> func), (override));
    MOCK_METHOD(void, setInputHandler, (caps_log::view::InputHandlerBase * handler), (override));
    MOCK_METHOD(caps_log::view::PopUpViewBase &, getPopUpView, (), (override));
    MOCK_METHOD(bool, handleInputEvent, (const caps_log::view::UIEvent &event), (override));
    MOCK_METHOD(void, switchLayout, (), (override));

    DummyAnnualViewLayout &getDummyAnnualViewLayout() const {
        return m_annualViewLayout->getDummyView();
    }

    DummyScratchpadViewLayout &getDummyScratchpadViewLayout() const {
        return m_scratchpadViewLayout->getDummyView();
    }
};

class DummyRepository : public caps_log::log::LogRepositoryBase {
    std::map<std::chrono::year_month_day, std::string> m_data;

  public:
    [[nodiscard]] std::optional<caps_log::log::LogFile>
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

class DummyScratchpadRepository : public caps_log::log::ScratchpadRepositoryBase {
  public:
    std::vector<caps_log::log::Scratchpad> m_scratchpads;

    [[nodiscard]] caps_log::log::Scratchpads read() const override { return m_scratchpads; }

    void remove(std::string name) override {
        m_scratchpads.erase(
            std::remove_if(m_scratchpads.begin(), m_scratchpads.end(),
                           [&name](const auto &spad) { return spad.title == name; }),
            m_scratchpads.end());
    }

    void rename(std::string oldName, std::string newName) override {
        for (auto &scratchpad : m_scratchpads) {
            if (scratchpad.title == oldName) {
                scratchpad.title = newName;
                return;
            }
        }
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

class DMockScratchpadRepo : public caps_log::log::ScratchpadRepositoryBase {
    DummyScratchpadRepository m_repo;

  public:
    DMockScratchpadRepo() {
        ON_CALL(*this, read).WillByDefault([this]() { return m_repo.read(); });
        ON_CALL(*this, remove).WillByDefault([this](const auto &name) { m_repo.remove(name); });
        ON_CALL(*this, rename).WillByDefault([this](const auto &oldName, const auto &newName) {
            m_repo.rename(oldName, newName);
        });
    }

    auto &getDummyRepo() { return m_repo; }

    MOCK_METHOD(caps_log::log::Scratchpads, read, (), (const, override));
    MOCK_METHOD(void, remove, (std::string name), (override));
    MOCK_METHOD(void, rename, (std::string oldName, std::string newName), (override));
};

class MockEditor : public caps_log::editor::EditorBase {
  public:
    MOCK_METHOD(void, openLog, (const caps_log::log::LogFile &), (override));
    MOCK_METHOD(void, openScratchpad, (const std::string &), (override));
};
