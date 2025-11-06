#include "app.hpp"
#include "utils/date.hpp"
#include "utils/string.hpp"
#include "view/view.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <memory>
#include <ranges>

namespace caps_log {

using namespace editor;
using namespace log;
using namespace view;
using namespace utils;

static const std::string kHelpString = R"(
  Captain's Log (caps-log) - Help
  -------------------------------
  Version: )" + std::string{CAPS_LOG_VERSION_STRING} +
                                       R"(

  **Caption's Log** is a terminal-based application for maintaining daily logs/journals with 
  optional scratchpads/notes. 

  All files are stored in markdown format for easy readability and
  portability. Possible to integrate with git for version control, remote bacup, or rough approach to 
  sharing logs between multiple devices. And also supports basic encryption of log files for privacy.

  To learn more about configuration options, please refer to the documentation at: 
  https://github.com/nikoladucak/caps-log

  It supports "tagging" by using `* tagname` syntax within log entries, allowing for categorization 
  and filtering of logs based on user-defined tags. Additionally, logs can be organized into
  "sections" (e.g., work, personal, health) for better management and retrieval by using `# sectionname` syntax.

  The application can highlight dates with specific tags or sections in the calendar view, 
  making it easier to identify and access relevant logs. To do so, simply 
  select the desired section and/or tag from the lists to the left side of the calendar.

  You may notice that by selecting a section, the tag list is updated to only show tags that 
  exist within the selected section.

  # Controls:

  |---------------------------------------------------------------------|
  | Key(s)                     | Action                                 |
  |----------------------------|----------------------------------------|
  | F1                         | Show this help screen                  |
  | ↑/↓/←/→ or h/j/k/l         | Navigate dates with logs               |
  | +/-                        | Change displayed year                  |
  | Enter                      | Open log for focused date              |
  | s                          | Open scratchpad view                   |
  | d                          | Delete focused scratchpad/log          |
  | r                          | Rename focused scratchpad              |
  | q/Escape                   | Quit application                       |
  |---------------------------------------------------------------------|
  )";

namespace {

[[nodiscard]] std::string exceptionPtrToString(const std::exception_ptr &ptr) {
    try {
        if (ptr) {
            std::rethrow_exception(ptr);
        }
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "Unknown exception";
    }
    return "No exception";
}

[[nodiscard]] bool noMeaningfulContent(const std::string &content,
                                       const std::chrono::year_month_day &date) {
    return utils::trim(content) == date::formatToString(date, kLogBaseTemplate) || content.empty();
}

[[nodiscard]] std::string makePreviewTitle(std::chrono::year_month_day date,
                                           const CalendarEvents &events) {
    auto eventsForDate = [&]() -> std::map<std::string, std::set<std::string>> {
        std::map<std::string, std::set<std::string>> filteredEvents;

        for (const auto &[groupName, groupEvents] : events) {
            for (const auto &event : groupEvents) {
                if (event.date == date::monthDay(date)) {
                    filteredEvents[groupName].insert(event.name);
                }
            }
        }
        return filteredEvents;
    }();

    // join in format: "birthdays: alice, bob; holidays: christmas"
    std::string eventString;
    for (const auto &[group, events] : eventsForDate) {
        if (not eventString.empty()) {
            eventString += "; ";
        }
        eventString += fmt::format("{}: {}", group, fmt::join(events, ", "));
    }

    const auto title = fmt::format("log preview for {} {}", utils::date::formatToString(date),
                                   eventString.empty() ? "" : " - " + eventString);
    return title;
}

[[nodiscard]] auto makeViewModelForScratchpads(const std::vector<Scratchpad> &scratchpads) {
    std::vector<view::ScratchpadData> viewScratchpads;
    viewScratchpads.reserve(scratchpads.size());

    for (const auto &scratchpad : scratchpads) {
        viewScratchpads.emplace_back(view::ScratchpadData{
            .title = scratchpad.title,
            .content = scratchpad.content,
            .dateModified = scratchpad.dateModified,
        });
    }
    return viewScratchpads;
}

} // namespace

ViewDataUpdater::ViewDataUpdater(std::shared_ptr<AnnualViewLayoutBase> view,
                                 const AnnualLogData &data)
    : m_view{std::move(view)}, m_data{data} {}

void ViewDataUpdater::handleFocusedTagChange() {
    const auto newTag = m_view->getSelectedTag();
    if (m_view->getSelectedSection() == kSelectNoneMenuEntryText) {
        if (newTag == kSelectNoneMenuEntryText) {
            m_view->setHighlightedDates(nullptr);
        } else {
            m_view->setHighlightedDates(
                &m_data.tagsPerSection.at(AnnualLogData::kAnySection).at(newTag));
        }
    } else if (newTag == kSelectNoneMenuEntryText) {
        m_view->setHighlightedDates(
            &m_data.tagsPerSection.at(m_view->getSelectedSection()).at(AnnualLogData::kAnyOrNoTag));
    } else {
        const auto *const highlightedDates =
            &m_data.tagsPerSection.at(m_view->getSelectedSection()).at(newTag);
        m_view->setHighlightedDates(highlightedDates);
    }
}

void ViewDataUpdater::handleFocusedSectionChange() {
    const auto newSection = m_view->getSelectedSection();
    m_view->setSelectedTag(kSelectNoneMenuEntryText);
    if (newSection == kSelectNoneMenuEntryText) {
        m_view->tagMenuItems() = m_tagMenuItemsPerSection.at(kSelectNoneMenuEntryText);
        m_view->setHighlightedDates(nullptr);
    } else {
        m_view->tagMenuItems() = m_tagMenuItemsPerSection.at(newSection);
        m_view->setHighlightedDates(
            &m_data.tagsPerSection.at(newSection).at(AnnualLogData::kAnyOrNoTag));
    }
}

void ViewDataUpdater::updateTagMenuItemsPerSection() {
    m_tagMenuItemsPerSection.clear();

    // update menu items
    const auto allTags = m_data.getAllTags();
    m_tagMenuItemsPerSection[kSelectNoneMenuEntryText] = makeTagMenuItems(kSelectNoneMenuEntryText);

    for (const auto &section : m_data.getAllSections()) {
        m_tagMenuItemsPerSection[section] = makeTagMenuItems(section);
    }
}

void ViewDataUpdater::updateViewAfterDataChange(const std::string &previewTitle,
                                                const std::string &previewString) {
    // update sections menu items
    {
        const auto oldSections = m_view->sectionMenuItems().getKeys();
        if (oldSections.empty()) { // initial start
            m_view->sectionMenuItems() = makeSectionMenuItems();
            m_view->setSelectedSection(kSelectNoneMenuEntryText);
        } else {
            const auto oldSelectedSection = m_view->getSelectedSection();
            m_view->sectionMenuItems() = makeSectionMenuItems();

            // restore selected section
            if (std::ranges::find(m_view->sectionMenuItems().getKeys(), oldSelectedSection) !=
                m_view->sectionMenuItems().getKeys().end()) {
                m_view->setSelectedSection(oldSelectedSection);
            } else {
                m_view->setSelectedSection(kSelectNoneMenuEntryText);
            }
        }
    }

    // update tags menu items
    {
        if (m_view->tagMenuItems().getKeys().empty()) { // initial start
            updateTagMenuItemsPerSection();
            m_view->tagMenuItems() = m_tagMenuItemsPerSection.at(m_view->getSelectedSection());
            m_view->setSelectedTag(kSelectNoneMenuEntryText);
        } else {
            // see if under the currently selected section there exists a tag that was selected
            // before
            const auto oldSelectedTag = m_view->getSelectedTag();
            updateTagMenuItemsPerSection();
            const auto newMenuItems = m_tagMenuItemsPerSection.at(m_view->getSelectedSection());
            m_view->tagMenuItems() = newMenuItems;
            if (std::ranges::find(newMenuItems.getKeys(), oldSelectedTag) !=
                newMenuItems.getKeys().end()) {
                m_view->setSelectedTag(oldSelectedTag);
            } else {
                m_view->setSelectedTag(kSelectNoneMenuEntryText);
            }
        }
    }

    // update highlighted dates

    if (m_view->getSelectedTag() == kSelectNoneMenuEntryText &&
        m_view->getSelectedSection() == kSelectNoneMenuEntryText) {
        m_view->setHighlightedDates(nullptr);
    } else if (m_view->getSelectedTag() == kSelectNoneMenuEntryText) {
        m_view->setHighlightedDates(
            &m_data.tagsPerSection.at(m_view->getSelectedSection()).at(AnnualLogData::kAnyOrNoTag));
    } else {
        m_view->setHighlightedDates(
            &m_data.tagsPerSection.at(AnnualLogData::kAnySection).at(m_view->getSelectedTag()));
    }

    // update preview string
    m_view->setPreviewString(previewTitle, previewString);
}

MenuItems ViewDataUpdater::makeTagMenuItems(const std::string &section) {
    std::vector<std::string> menuItems;
    std::vector<std::string> keys;

    // prepend select none
    menuItems.push_back(kSelectNoneMenuEntryText);
    keys.push_back(kSelectNoneMenuEntryText);

    const auto sect = section == kSelectNoneMenuEntryText ? AnnualLogData::kAnySection : section;
    for (const auto &[tag, dates] : m_data.tagsPerSection.at(sect)) {
        if (tag == AnnualLogData::kAnyOrNoTag) {
            continue;
        }
        menuItems.push_back(makeMenuItemTitle(tag, dates.size()));
        keys.push_back(tag);
    }
    return {menuItems, keys};
}

MenuItems ViewDataUpdater::makeSectionMenuItems() {
    std::vector<std::string> menuItems;
    std::vector<std::string> keys;
    menuItems.reserve(m_data.tagsPerSection.size());
    keys.reserve(m_data.tagsPerSection.size());

    // prepend select none
    menuItems.push_back(kSelectNoneMenuEntryText);
    keys.push_back(kSelectNoneMenuEntryText);

    for (const auto &section : m_data.tagsPerSection | std::views::keys) {
        if (section == AnnualLogData::kAnySection) {
            continue;
        }
        menuItems.push_back(makeMenuItemTitle(
            section, m_data.tagsPerSection.at(section).at(AnnualLogData::kAnyOrNoTag).size()));
        keys.push_back(section);
    }
    return {menuItems, keys};
}

void App::updateDataAndViewAfterLogChange(const std::chrono::year_month_day &dateOfChangedLog) {
    m_data.collect(m_repo, dateOfChangedLog, m_config.skipFirstLine);
    std::string previewString;
    if (dateOfChangedLog == m_view->getAnnualViewLayout()->getFocusedDate()) {
        if (auto log = m_repo->read(m_view->getAnnualViewLayout()->getFocusedDate())) {
            previewString = log->getContent();
        }
    }

    m_viewDataUpdater.updateViewAfterDataChange(makePreviewTitle(dateOfChangedLog, m_config.events),
                                                previewString);
}

App::App(std::shared_ptr<ViewBase> view, std::shared_ptr<LogRepositoryBase> repo,
         std::shared_ptr<ScratchpadRepositoryBase> scratchpadRepo,
         std::shared_ptr<EditorBase> editor, std::optional<GitRepo> gitRepo, AppConfig config)
    : m_config{std::move(config)}, m_view{std::move(view)}, m_repo{std::move(repo)},
      m_scratchpadRepo{std::move(scratchpadRepo)}, m_editor{std::move(editor)},
      m_data{AnnualLogData::collect(m_repo, m_config.currentYear, m_config.skipFirstLine)},
      m_viewDataUpdater{m_view->getAnnualViewLayout(), m_data} {
    m_view->setInputHandler(this);
    m_view->getAnnualViewLayout()->setDatesWithLogs(&m_data.datesWithLogs);
    m_view->getAnnualViewLayout()->setEventDates(&m_config.events);
    updateDataAndViewAfterLogChange(m_view->getAnnualViewLayout()->getFocusedDate());

    if (gitRepo) {
        m_gitRepo.emplace(std::move(*gitRepo));
    }
}

void App::run() { m_view->run(); }

bool App::handleInputEvent(const UIEvent &event) {
    return std::visit(
        [this](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, DisplayedYearChange>) {
                handleDisplayedYearChange(arg.year);
            } else if constexpr (std::is_same_v<T, OpenLogFile>) {
                handleOpenLogFile();
            } else if constexpr (std::is_same_v<T, OpenScratchpad>) {
                handleOpenScratchpad(arg.name);
            } else if constexpr (std::is_same_v<T, DeleteScratchpad>) {
                handleDeleteScratchpad(arg.name);
            } else if constexpr (std::is_same_v<T, RenameScratchpad>) {
                handleRenameScratchpad(arg.name);
            } else if constexpr (std::is_same_v<T, UiStarted>) {
                handleUiStarted();
            } else if constexpr (std::is_same_v<T, FocusedDateChange>) {
                handleFocusedDateChange();
            } else if constexpr (std::is_same_v<T, FocusedTagChange>) {
                handleFocusedTagChange();
            } else if constexpr (std::is_same_v<T, FocusedSectionChange>) {
                handleFocusedSectionChange();
            } else if constexpr (std::is_same_v<T, UnhandledRootEvent>) {
                return handleRootEvent(arg.input);
            }
            return true;
        },
        event);
};

bool App::handleRootEvent(const std::string &input) {
    if (input == "\x1B" || input == "q") {
        quit();
    } else if (input == "d") {
        deleteFocusedLog();
    } else if (input == "+") {
        handleDisplayedYearChange(+1);
    } else if (input == "-") {
        handleDisplayedYearChange(-1);
    } else if (input == "s") {
        handleSwitchLayout();
    } else if (input == ftxui::Event::F1.input()) {
        m_view->getPopUpView().show(PopUpViewBase::Help{kHelpString});
    } else {
        return false;
    }

    return true;
}

void App::handleSwitchLayout() {
    // todo: switch to scratchpad view if not already there
    auto scratchapds = m_scratchpadRepo->read();
    m_view->getScratchpadViewLayout()->setScratchpads(makeViewModelForScratchpads(scratchapds));
    m_view->switchLayout();
}

void App::handleFocusedDateChange() {
    const auto title =
        makePreviewTitle(m_view->getAnnualViewLayout()->getFocusedDate(), m_config.events);
    if (auto log = m_repo->read(m_view->getAnnualViewLayout()->getFocusedDate())) {
        m_view->getAnnualViewLayout()->setPreviewString(title, log->getContent());
    } else {
        m_view->getAnnualViewLayout()->setPreviewString(title, "");
    }
}

void App::handleFocusedTagChange() { m_viewDataUpdater.handleFocusedTagChange(); }

void App::handleFocusedSectionChange() { m_viewDataUpdater.handleFocusedSectionChange(); }

void App::handleUiStarted() {
    if (!m_gitRepo) {
        return;
    }
    m_view->getPopUpView().show(PopUpViewBase::Loading{"Pulling from remote..."});
    m_gitRepo->pull([this](std::expected<void, std::exception_ptr> result) {
        m_view->post([this, result = std::move(result)]() {
            if (!result.has_value()) {
                m_view->getPopUpView().show(PopUpViewBase::Ok{fmt::format(
                    "Error pulling from remote:\n{}", exceptionPtrToString(result.error()))});
            } else {
                m_data =
                    AnnualLogData::collect(m_repo, m_config.currentYear, m_config.skipFirstLine);
                updateDataAndViewAfterLogChange(m_view->getAnnualViewLayout()->getFocusedDate());
                m_view->getPopUpView().show(PopUpViewBase::None{});
            }
        });
    });
}

void App::quit() {
    if (!m_gitRepo) {
        m_view->stop();
        return;
    }
    m_view->getPopUpView().show(PopUpViewBase::Loading{"Committing & pushing..."});
    m_gitRepo->commitAll([this](std::expected<bool, std::exception_ptr> result) {
        if (result.has_value()) {
            const bool somethingCommitted = result.value();
            if (somethingCommitted) {
                m_gitRepo->push([this](std::expected<void, std::exception_ptr> pushResult) {
                    if (pushResult.has_value()) {
                        m_view->stop();
                    } else {
                        m_view->post([this, pushResult = std::move(pushResult)]() {
                            m_view->getPopUpView().show(PopUpViewBase::Ok{
                                fmt::format("Error pushing to remote:\n{}",
                                            exceptionPtrToString(pushResult.error())),
                                [this](const auto &) { m_view->stop(); }});
                        });
                    }
                });
            } else {
                m_view->post([this]() { m_view->stop(); });
            }
        } else {
            m_view->post([this, result = std::move(result)]() {
                m_view->getPopUpView().show(
                    PopUpViewBase::Ok{fmt::format("Error committing to remote:\n{}",
                                                  exceptionPtrToString(result.error())),
                                      [this](const auto &) { m_view->stop(); }});
            });
        }
    });
};

void App::deleteFocusedLog() {
    auto date = m_view->getAnnualViewLayout()->getFocusedDate();
    if (m_data.datesWithLogs.contains(date::monthDay(date))) {
        m_view->getPopUpView().show(PopUpViewBase::YesNo{
            "Are you sure you want to delete a log file?", [date, this](const auto &result) {
                if (std::holds_alternative<PopUpViewBase::Result::Yes>(result)) {
                    m_repo->remove(date);
                    updateDataAndViewAfterLogChange(date);
                }
            }});
    }
}

void App::handleDisplayedYearChange(int diff) {
    m_config.currentYear = std::chrono::year{static_cast<int>(m_config.currentYear) + diff};
    m_data = AnnualLogData::collect(m_repo, m_config.currentYear, m_config.skipFirstLine);
    m_view->getAnnualViewLayout()->showCalendarForYear(m_config.currentYear);
    m_view->getAnnualViewLayout()->setHighlightedDates(nullptr);
    updateDataAndViewAfterLogChange(m_view->getAnnualViewLayout()->getFocusedDate());
}

bool hasSuffix(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
void App::handleOpenScratchpad(std::string name) {
    if (m_editor == nullptr) {
        return;
    }

    if (name.empty()) {
        m_view->getPopUpView().show(PopUpViewBase::TextBox{
            "Enter the name of the scratchpad to open:", [this](const auto &name) {
                if (std::holds_alternative<PopUpViewBase::Result::Input>(name)) {
                    auto nameStr = std::get<PopUpViewBase::Result::Input>(name).text;
                    if (nameStr.empty()) {
                        m_view->getPopUpView().show(
                            PopUpViewBase::Ok{"Scratchpad name cannot be empty."});
                        return;
                    }
                    nameStr = utils::trim(nameStr);

                    // if name doesn't end with .md, add it
                    if (!hasSuffix(nameStr, ".md")) {
                        nameStr += ".md";
                    }

                    handleOpenScratchpad(nameStr);
                }
            }});
    } else {
        m_view->withRestoredIO([this, &name]() {
            m_editor->openScratchpad(name);
            auto scratchpads = m_scratchpadRepo->read();
            m_view->getScratchpadViewLayout()->setScratchpads(
                makeViewModelForScratchpads(scratchpads));
        });
    }
}

void App::handleRenameScratchpad(std::string name) {
    if (name.empty()) {
        return;
    }
    auto message = fmt::format("Enter the new name for the scratchpad \"{}\":", name);

    m_view->getPopUpView().show(PopUpViewBase::TextBox{
        message, [this, name](const auto &input) {
            if (std::holds_alternative<PopUpViewBase::Result::Input>(input)) {
                auto newName = std::get<PopUpViewBase::Result::Input>(input).text;
                if (newName.empty()) {
                    m_view->getPopUpView().show(
                        PopUpViewBase::Ok{"Scratchpad name cannot be empty."});
                    return;
                }
                newName = utils::trim(newName);

                // if name doesn't end with .md, add it
                if (!hasSuffix(newName, ".md")) {
                    newName += ".md";
                }

                m_scratchpadRepo->rename(name, newName);
                auto scratchpads = m_scratchpadRepo->read();
                m_view->getScratchpadViewLayout()->setScratchpads(
                    makeViewModelForScratchpads(scratchpads));
            }
        }});
}

void App::handleDeleteScratchpad(std::string name) {
    if (name.empty()) {
        return;
    }

    m_view->getPopUpView().show(PopUpViewBase::YesNo{
        fmt::format("Are you sure you want to delete the scratchpad \"{}\"?", name),
        [this, name](const auto &result) {
            if (std::holds_alternative<PopUpViewBase::Result::Yes>(result)) {
                m_scratchpadRepo->remove(name);
                m_view->getScratchpadViewLayout()->setScratchpads(
                    makeViewModelForScratchpads(m_scratchpadRepo->read()));
            }
        }});
}

void App::handleOpenLogFile() {
    if (m_editor == nullptr) {
        return;
    }

    // otherwise, open the editor for the focused log
    auto date = m_view->getAnnualViewLayout()->getFocusedDate();
    auto log = m_repo->read(date);
    if (not log.has_value()) {
        m_repo->write({date, date::formatToString(date, kLogBaseTemplate)});

        // this looks a little awkward, it's easy to forget to reread after write
        log = m_repo->read(date);
    }

    assert(log);
    m_view->withRestoredIO([this, &log, date]() {
        m_editor->openLog(*log);

        // check that after editing still exists
        log = m_repo->read(date);
        if (log && noMeaningfulContent(log->getContent(), date)) {
            m_repo->remove(date);
        }
        updateDataAndViewAfterLogChange(date);
    });
}

} // namespace caps_log
