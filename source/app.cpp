#include "app.hpp"
#include "utils/date.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <iostream>
#include <memory>

namespace caps_log {

using namespace editor;
using namespace log;
using namespace view;
using namespace utils;

ViewDataUpdater::ViewDataUpdater(std::shared_ptr<AnnualViewBase> view, const AnnualLogData &data)
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
            &m_data.tagsPerSection.at(m_view->getSelectedSection()).at(AnnualLogData::kAnyTag));
    } else {
        const auto *const highlighMap =
            &m_data.tagsPerSection.at(m_view->getSelectedSection()).at(newTag);
        m_view->setHighlightedDates(highlighMap);
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
            &m_data.tagsPerSection.at(newSection).at(AnnualLogData::kAnyTag));
    }
}

void ViewDataUpdater::updateTagMenuItemsPerSection() {
    m_tagMenuItemsPerSection.clear();

    // update menu items
    const auto allTags = m_data.getAllTags();
    m_tagMenuItemsPerSection[kSelectNoneMenuEntryText] =
        makeTagMenuItems(allTags, kSelectNoneMenuEntryText);

    // NOTE: gotta itterate over datesWithSection to include sections that do not have tags as they
    // are not included in tagsPerSection
    for (const auto &section : m_data.getAllSections()) {
        if (m_data.tagsPerSection.contains(section)) {
            m_tagMenuItemsPerSection[section] =
                makeTagMenuItems(m_data.getAllTagsForSection(section), section);
        } else {
            m_tagMenuItemsPerSection[section] = makeTagMenuItems({}, section);
        }
    }
}

void ViewDataUpdater::updateViewAfterDataChange(const std::string &previewString) {
    // update sections menu items
    {
        const auto oldSections = m_view->sectionMenuItems().getKeys();
        if (oldSections.empty()) { // initial start
            std::cout << "initial start" << m_data.tagsPerSection.begin()->first << std::endl;
            m_view->sectionMenuItems() = makeSectionMenuItems(m_data.getAllSections());
            m_view->setSelectedSection(kSelectNoneMenuEntryText);
        } else {
            const auto oldSelectedSection = m_view->getSelectedSection();
            m_view->sectionMenuItems() = makeSectionMenuItems(m_data.getAllSections());

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
            &m_data.tagsPerSection.at(m_view->getSelectedSection()).at(AnnualLogData::kAnyTag));
    } else {
        m_view->setHighlightedDates(
            &m_data.tagsPerSection.at(AnnualLogData::kAnySection).at(m_view->getSelectedTag()));
    }

    // update preview string
    m_view->setPreviewString(previewString);
}

void App::updateDataAndViewAfterLogChange(const std::chrono::year_month_day &dateOfChangedLog) {
    std::cout << "PRE " << m_data.tagsPerSection.size() << std::endl;
    std::cout << "date " << date::formatToString(dateOfChangedLog) << std::endl;
    m_data.collect(m_repo, dateOfChangedLog, m_skipFirstLine);
    std::cout << "POST " << m_data.tagsPerSection.size() << std::endl;
    std::string previewString = "";
    if (dateOfChangedLog == m_view->getFocusedDate()) {
        if (auto log = m_repo->read(m_view->getFocusedDate())) {
            previewString = log->getContent();
        }
    }
    m_viewDataUpdater.updateViewAfterDataChange(previewString);
}

App::App(std::shared_ptr<AnnualViewBase> view, std::shared_ptr<LogRepositoryBase> repo,
         std::shared_ptr<EditorBase> editor, bool skipFirstLine, std::optional<GitRepo> gitRepo,
         std::chrono::year year)
    : m_displayedYear{year}, m_view{std::move(view)}, m_repo{std::move(repo)},
      m_editor{std::move(editor)},
      m_data{AnnualLogData::collect(m_repo, m_displayedYear, skipFirstLine)},
      m_skipFirstLine{skipFirstLine}, m_viewDataUpdater{m_view, m_data} {
    m_view->setInputHandler(this);
    m_view->setDatesWithLogs(&m_data.datesWithLogs);
    updateDataAndViewAfterLogChange(m_view->getFocusedDate());

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
                handleCalendarButtonClick();
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
    } else {
        return false;
    }

    return true;
}

void App::handleFocusedDateChange() {
    if (auto log = m_repo->read(m_view->getFocusedDate())) {
        m_view->setPreviewString(log->getContent());
    } else {
        m_view->setPreviewString("");
    }
}

void App::handleFocusedTagChange() { m_viewDataUpdater.handleFocusedTagChange(); }

void App::handleFocusedSectionChange() { m_viewDataUpdater.handleFocusedSectionChange(); }

void App::handleUiStarted() {
    if (m_gitRepo) {
        m_view->loadingScreen("Pulling from remote...");
        m_gitRepo->pull([this] {
            m_view->post([this] {
                m_data = AnnualLogData::collect(m_repo, m_displayedYear, m_skipFirstLine);
                updateDataAndViewAfterLogChange(m_view->getFocusedDate());
                m_view->loadingScreenOff();
            });
        });
    }
}

void App::quit() {
    if (m_gitRepo) {
        m_view->loadingScreen("Committing & pushing...");
        m_gitRepo->commitAll([this](bool somethingCommited) {
            if (somethingCommited) {
                m_gitRepo->push([this] { m_view->stop(); });
            } else {
                m_view->stop();
            }
        });
    } else {
        m_view->stop();
    }
}

void App::deleteFocusedLog() {
    auto date = m_view->getFocusedDate();
    if (m_data.datesWithLogs.contains(date::monthDay(date))) {
        m_view->prompt("Are you sure you want to delete a log file?", [date, this] {
            m_repo->remove(date);
            updateDataAndViewAfterLogChange(date);
        });
    }
}

void App::handleDisplayedYearChange(int diff) {
    m_displayedYear = std::chrono::year{static_cast<int>(m_displayedYear) + diff};
    m_data = AnnualLogData::collect(m_repo, m_displayedYear, m_skipFirstLine);
    m_view->showCalendarForYear(m_displayedYear);
    m_view->setHighlightedDates(nullptr);
    updateDataAndViewAfterLogChange(m_view->getFocusedDate());
}

void App::handleCalendarButtonClick() {
    if (m_editor == nullptr) {
        return;
    }
    m_view->withRestoredIO([this]() {
        auto date = m_view->getFocusedDate();
        auto log = m_repo->read(date);
        if (not log.has_value()) {
            m_repo->write({date, date::formatToString(date, kLogBaseTemplate)});
            // this looks a little awkward, it's easy to forget to reread after write
            log = m_repo->read(date);
        }

        assert(log);
        m_editor->openEditor(*log);

        // check that after editing still exists
        log = m_repo->read(date);
        if (log && noMeaningfullContent(log->getContent(), date)) {
            m_repo->remove(date);
        }
        updateDataAndViewAfterLogChange(date);
    });
}

bool App::noMeaningfullContent(const std::string &content,
                               const std::chrono::year_month_day &date) {
    return content == date::formatToString(date, kLogBaseTemplate) || content.empty();
}
} // namespace caps_log
