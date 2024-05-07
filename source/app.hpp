#pragma once

#include "editor/editor_base.hpp"
#include "log/annual_log_data.hpp"
#include "log/log_repository_base.hpp"
#include "utils/git_repo.hpp"
#include "utils/task_executor.hpp"
#include "view/annual_view_base.hpp"
#include "view/input_handler.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace caps_log {

using namespace editor;
using namespace log;
using namespace view;
using namespace utils;

static const std::string kLogBaseTemplate{"# %d. %m. %y."};

/**
 * A class that has the interface of GitRepo but extends each method to provide callback
 * functionality while executing real git log repo stuff on ThreadedTaskExecutor.
 * It ensures that only one thread is touching the repo.
 */
class AsyncGitRepo {
    GitRepo m_repo;
    utils::ThreadedTaskExecutor m_taskExec;

  public:
    explicit AsyncGitRepo(GitRepo repo) : m_repo{std::move(repo)} {}

    void pull(std::function<void()> acallback) {
        m_taskExec.post([this, callback = std::move(acallback)] {
            m_repo.pull();
            callback();
        });
    }

    void push(std::function<void()> acallback) {
        m_taskExec.post([this, callback = std::move(acallback)]() {
            m_repo.push();
            callback();
        });
    }

    void commitAll(std::function<void(bool)> acallback) {
        m_taskExec.post([this, callback = std::move(acallback)]() {
            auto somethingCommited = m_repo.commitAll();
            callback(somethingCommited);
        });
    }
};

class App : public InputHandlerBase {
    std::chrono::year m_displayedYear;
    std::shared_ptr<AnnualViewBase> m_view;
    std::shared_ptr<LogRepositoryBase> m_repo;
    std::shared_ptr<EditorBase> m_editor;
    AnnualLogData m_data;

    std::vector<const date::AnnualMap<bool> *> m_tagMaps;
    std::vector<const date::AnnualMap<bool> *> m_sectionMaps;
    bool m_skipFirstLine;

    std::optional<AsyncGitRepo> m_gitRepo;

    void
    updateViewSectionsAndTagsAfterLogChange(const std::chrono::year_month_day &dateOfChangedLog) {
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
        std::optional<GitRepo> gitRepo = std::nullopt,
        std::chrono::year year = date::getToday().year())
        : m_displayedYear{year}, m_view{std::move(view)}, m_repo{std::move(repo)},
          m_editor{std::move(editor)},
          m_data{AnnualLogData::collect(m_repo, m_displayedYear, skipFirstLine)},
          m_skipFirstLine{skipFirstLine} {
        m_view->setInputHandler(this);
        m_view->setAvailableLogsMap(&m_data.logAvailabilityMap);
        updateViewSectionsAndTagsAfterLogChange(m_view->getFocusedDate());

        if (gitRepo) {
            m_gitRepo.emplace(std::move(*gitRepo));
        }
    }

    void run() { m_view->run(); }

    bool handleInputEvent(const UIEvent &event) override {
        return std::visit(
            [this](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, DisplayedYearChange>) {
                    displayYear(arg.year);
                } else if constexpr (std::is_same_v<T, OpenLogFile>) {
                    handleCalendarButtonClick();
                } else if constexpr (std::is_same_v<T, UiStarted>) {
                    handleUiStarted();
                } else if constexpr (std::is_same_v<T, FocusedDateChange>) {
                    handleFocusedDateChange(arg.date);
                } else if constexpr (std::is_same_v<T, FocusedTagChange>) {
                    handleFocusedTagChange(arg.tag);
                } else if constexpr (std::is_same_v<T, FocusedSectionChange>) {
                    handleFocusedSectionChange(arg.section);
                } else if constexpr (std::is_same_v<T, UnhandledRootEvent>) {
                    return handleRootEvent(arg.input);
                }
                return true;
            },
            event);
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

    void handleFocusedDateChange(const std::chrono::year_month_day &date) {
        if (auto log = m_repo->read(date)) {
            m_view->setPreviewString(log->getContent());
        } else {
            m_view->setPreviewString("");
        }
    }

    void handleFocusedTagChange(int newTag) {
        m_view->selectedSection() = 0;
        const auto *const highlighMap = m_tagMaps.at(newTag);
        m_view->setHighlightedLogsMap(highlighMap);
    }

    void handleFocusedSectionChange(int newSection) {
        m_view->selectedTag() = 0;
        const auto *const highlighMap = m_sectionMaps.at(newSection);
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
        m_displayedYear = std::chrono::year{(int)m_displayedYear + diff};
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
            updateViewSectionsAndTagsAfterLogChange(date);
        });
    }

    static bool noMeaningfullContent(const std::string &content,
                                     const std::chrono::year_month_day &date) {
        return content == date::formatToString(date, kLogBaseTemplate) || content.empty();
    }

    static const date::AnnualMap<bool> *findOrNull(const date::StringYearMap &map,
                                                   const std::string &key) {
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
    static void updatePointersForHighlightMaps(std::vector<const date::AnnualMap<bool> *> &vec,
                                               const date::StringYearMap &map) {
        vec.clear();
        vec.reserve(map.size());
        vec.push_back(nullptr); // 0 index = no highlighted days
        for (auto const &[_, annual_map] : map) {
            vec.emplace_back(&annual_map);
        }
    }

    /**
     * Generates a list of strings that will be displayed as menu items. Each item
     * title will be made of a key in the @p map and the number of days it has been
     * mantioned (days set).
     */
    static std::vector<std::string> makeMenuTitles(const date::StringYearMap &map) {
        std::vector<std::string> strs;
        strs.reserve(map.size());
        strs.push_back(" ----- ");
        for (auto const &[str, annual_map] : map) {
            strs.push_back(std::string{"("} + std::to_string(annual_map.daysSet()) + ") - " + str);
        }
        return std::move(strs);
    }
};
} // namespace caps_log
