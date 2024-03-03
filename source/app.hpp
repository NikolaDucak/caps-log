#pragma once

#include "editor/editor_base.hpp"
#include "log/annual_log_data.hpp"
#include "log/log_file.hpp"
#include "log/log_repository_base.hpp"
#include "utils/git_repo.hpp"
#include "utils/string.hpp"
#include "utils/task_executor.hpp"
#include "view/annual_view.hpp"
#include "view/input_handler.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

namespace caps_log {

using namespace editor;
using namespace log;
using namespace view;
using namespace utils;

inline static const std::string LOG_BASE_TEMPLATE{"# %d. %m. %y."};

/**
 * A class that has the interface of GitRepo but extends each method to provide callback
 * functionality while executing real git log repo stuff on ThreadedTaskExecutor.
 * It ensures that only one thread is touching the repo.
 */
class AsyncGitRepo {
    GitRepo m_repo;
    utils::ThreadedTaskExecutor m_taskExec;

  public:
    AsyncGitRepo(GitRepo repo) : m_repo{std::move(repo)} {}

    void pull(std::function<void()> acallback) {
        m_taskExec.Post([this, callback = std::move(acallback)] {
            m_repo.pull();
            callback();
        });
    }

    void push(std::function<void()> acallback) {
        m_taskExec.Post([this, callback = std::move(acallback)]() {
            m_repo.push();
            callback();
        });
    }

    void commitAll(std::function<void(bool)> acallback) {
        m_taskExec.Post([this, callback = std::move(acallback)]() {
            auto somethingCommited = m_repo.commitAll();
            callback(somethingCommited);
        });
    }
};

/**
 * The main caps-log class.
 * It's the main class that holds the input handling logic and the main loop.
 * It's constructed with a view, repository and editor. It's responsible for
 * updating the view with the data from the repository and handling user input.
 */
class App : public InputHandlerBase {
    unsigned m_displayedYear;
    std::shared_ptr<AnnualViewBase> m_view;
    std::shared_ptr<LogRepositoryBase> m_repo;
    std::shared_ptr<EditorBase> m_editor;
    AnnualLogData m_data;

    std::vector<const YearMap<bool> *> m_tagMaps;
    std::vector<const YearMap<bool> *> m_sectionMaps;
    bool m_skipFirstLine;

    std::optional<AsyncGitRepo> m_gitRepo;

    void updateViewSectionsAndTagsAfterLogChange(const Date &dateOfChangedLog) {
        m_data.collect(m_repo, dateOfChangedLog, m_skipFirstLine);
        m_view->setTagMenuItems(makeMenuTitles(m_data.tagMap));
        m_view->setSectionMenuItems(makeMenuTitles(m_data.sectionMap));

        updatePointersForHighlightMaps(m_tagMaps, m_data.tagMap);
        updatePointersForHighlightMaps(m_sectionMaps, m_data.sectionMap);
        if (dateOfChangedLog == m_view->getFocusedDate()) {
            if (auto log = m_repo->read(m_view->getFocusedDate())) {
                m_view->setPreviewString(log->getContent());
            } else {
                m_view->setPreviewString("");
            }
        }
    }

  public:
    App(std::shared_ptr<AnnualViewBase> view, std::shared_ptr<LogRepositoryBase> repo,
        std::shared_ptr<EditorBase> editor, bool skipFirstLine = true,
        std::optional<GitRepo> gitRepo = std::nullopt)
        : m_displayedYear(Date::getToday().year), m_view{std::move(view)}, m_repo{std::move(repo)},
          m_editor{std::move(editor)},
          m_data{AnnualLogData::collect(m_repo, date::Date::getToday().year, skipFirstLine)},
          m_skipFirstLine{skipFirstLine} {
        m_view->setInputHandler(this);
        // if pass not prowided and repo is encrypted
        m_view->setAvailableLogsMap(&m_data.logAvailabilityMap);
        updateViewSectionsAndTagsAfterLogChange(m_view->getFocusedDate());

        if (gitRepo) {
            m_gitRepo.emplace(std::move(*gitRepo));
        }
    }

    void run() { m_view->run(); }

    bool handleInputEvent(const UIEvent &event) override {
        switch (event.type) {
        case UIEvent::ROOT_EVENT:
            return handleRootEvent(event.input);
        case UIEvent::UI_STARTED:
            handleUiStarted();
            break;
        case UIEvent::FOCUSED_DATE_CHANGE:
            handleFocusedDateChange(event.input);
            break;
        case UIEvent::FOCUSED_TAG_CHANGE:
            handleFocusedTagChange(event.input);
            break;
        case UIEvent::FOCUSED_SECTION_CHANGE:
            handleFocusedSectionChange(event.input);
            break;
        case UIEvent::CALENDAR_BUTTON_CLICK:
            handleCalendarButtonClick();
            break;
        };
        return true;
    };

  private:
    bool handleRootEvent(const std::string &input) {
        if (input == "\x1B") {
            quit();
        } else if (input == "d") {
            deleteFocusedLog();
        } else if (input == "+") {
            displayYear(+1);
        } else if (input == "-") {
            displayYear(-1);
        } else {
            return false;
        }

        return true;
    }

    void handleFocusedDateChange(const std::string & /* unused */) {
        if (auto log = m_repo->read(m_view->getFocusedDate())) {
            m_view->setPreviewString(log->getContent());
        } else {
            m_view->setPreviewString("");
        }
    }

    void handleFocusedTagChange(const std::string &newTag) {
        m_view->selectedSection() = 0;
        const auto *const highlighMap = m_tagMaps.at(std::stoi(newTag));
        m_view->setHighlightedLogsMap(highlighMap);
    }

    void handleFocusedSectionChange(const std::string &newSection) {
        m_view->selectedTag() = 0;
        const auto *const highlighMap = m_sectionMaps.at(std::stoi(newSection));
        m_view->setHighlightedLogsMap(highlighMap);
    }

    void handleUiStarted() {
        if (m_gitRepo) {
            m_view->loadingScreen("Pulling from remote...");
            m_gitRepo->pull([this] { m_view->post([this] { m_view->loadingScreenOff(); }); });
        }
    }

    void quit() {
        if (m_gitRepo) {
            m_view->loadingScreen("Commiting & pushing...");
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

    void deleteFocusedLog() {
        auto date = m_view->getFocusedDate();
        if (m_data.logAvailabilityMap.get(date)) {
            m_view->prompt("Are you sure you want to delete a log file?", [date, this] {
                m_repo->remove(date);
                updateViewSectionsAndTagsAfterLogChange(date);
            });
        }
    }

    void displayYear(int diff) {
        m_displayedYear += diff;
        m_data = AnnualLogData::collect(m_repo, m_displayedYear, m_skipFirstLine);
        m_view->showCalendarForYear(m_displayedYear);
        m_view->setHighlightedLogsMap(nullptr);
        updateViewSectionsAndTagsAfterLogChange(m_view->getFocusedDate());
    }

    void handleCalendarButtonClick() {
        m_view->withRestoredIO([this]() {
            auto date = m_view->getFocusedDate();
            auto log = m_repo->read(date);
            if (not log.has_value()) {
                m_repo->write({date, date.formatToString(LOG_BASE_TEMPLATE)});
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
            updateViewSectionsAndTagsAfterLogChange(date);
        });
    }

    static bool noMeaningfullContent(const std::string &content, const Date &date) {
        return content == date.formatToString(LOG_BASE_TEMPLATE) || content.empty();
    }

    static const YearMap<bool> *findOrNull(const StringYearMap &map, const std::string &key) {
        auto result = map.find(key);
        if (result == map.end()) {
            return nullptr;
        }
        return &result->second;
    }

    /**
     * A trick to optimize looking up selected menu item. View gives us an index
     * in the provided array of strings, we can use that index to lookup which availability
     * map for said menu item is appropriate. Alternative would getting the string and looking up
     * in the map.
     */
    static void updatePointersForHighlightMaps(std::vector<const YearMap<bool> *> &vec,
                                               const StringYearMap &map) {
        vec.clear();
        vec.reserve(map.size());
        vec.push_back(nullptr); // 0 index = no highlighted days
        for (auto const &imap : map) {
            vec.emplace_back(&imap.second);
        }
    }

    /**
     * Generates a list of strings that will be displayed as menu items. Each item
     * title will be made of a key in the @p map and the number of days it has been
     * mantioned (days set).
     */
    static std::vector<std::string> makeMenuTitles(const StringYearMap &map) {
        std::vector<std::string> strs;
        strs.reserve(map.size());
        strs.push_back(" ----- ");
        for (auto const &imap : map) {
            strs.push_back(std::string{"("} + std::to_string(imap.second.daysSet()) + ") - " +
                           imap.first);
        }
        return std::move(strs);
    }
};
} // namespace caps_log
