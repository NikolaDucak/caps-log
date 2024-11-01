#pragma once

#include "editor/editor_base.hpp"
#include "log/annual_log_data.hpp"
#include "log/log_repository_base.hpp"
#include "utils/git_repo.hpp"
#include "utils/task_executor.hpp"
#include "view/annual_view_base.hpp"
#include "view/input_handler.hpp"

#include <cassert>
#include <cstddef>
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

class MenuListItemMaintainer {
    std::shared_ptr<AnnualViewBase> m_view;
    std::vector<const utils::date::Dates *> m_tagMaps;
    std::vector<const utils::date::Dates *> m_sectionMaps;
    std::vector<std::string> m_tagMenuItems;
    std::vector<std::string> m_sectionMenuItems;

  public:
    MenuListItemMaintainer(std::shared_ptr<AnnualViewBase> view) : m_view{std::move(view)} {}

    void handleFocusedTagChange(int newTag) {
        m_view->selectedSection() = 0;
        const auto *const highlighMap = m_tagMaps.at(newTag);
        m_view->setHighlightedDates(highlighMap);
    }

    void handleFocusedSectionChange(int newSection) {
        m_view->selectedTag() = 0;
        const auto *const highlighMap = m_sectionMaps.at(newSection);
        m_view->setHighlightedDates(highlighMap);
    }

    void reselect(int &selected, const std::vector<std::string> &oldMenuItems,
                  const std::vector<std::string> &newMenuItems) {
        if (newMenuItems != oldMenuItems) {
            const auto &currentlySelectedTag = oldMenuItems[selected];
            const auto isStillInMenu = std::find(newMenuItems.begin(), newMenuItems.end(),
                                                 currentlySelectedTag) != newMenuItems.end();
            if (isStillInMenu) {
                selected = std::distance(
                    newMenuItems.begin(),
                    std::find(newMenuItems.begin(), newMenuItems.end(), currentlySelectedTag));
            } else {
                selected = 0;
            }
        }
    }

    void updateViewSectionsAndTagsAfterLogChange(const AnnualLogData &data,
                                                 const std::string &previewString) {
        const auto newTagMenuItems = makeMenuTitles(data.datesWithTag);
        const auto newSectionMenuItems = makeMenuTitles(data.datesWithSection);

        if (not m_tagMenuItems.empty()) {
            reselect(m_view->selectedTag(), m_tagMenuItems, newTagMenuItems);
        }
        if (not m_sectionMenuItems.empty()) {
            reselect(m_view->selectedSection(), m_sectionMenuItems, newSectionMenuItems);
        }

        m_tagMenuItems = newTagMenuItems;
        m_sectionMenuItems = newSectionMenuItems;

        m_view->setTagMenuItems(newTagMenuItems);
        m_view->setSectionMenuItems(newSectionMenuItems);

        updatePointersForHighlightMaps(m_tagMaps, data.datesWithTag);
        updatePointersForHighlightMaps(m_sectionMaps, data.datesWithSection);

        assert(m_view->selectedTag() == 0 ||
               m_view->selectedSection() == 0 && "Only one can be selected");
        if (m_view->selectedTag() == 0 && m_view->selectedSection() == 0) {
            m_view->setHighlightedDates(nullptr);
        }
        if (m_view->selectedTag() != 0) {
            m_view->setHighlightedDates(m_tagMaps.at(m_view->selectedTag()));
        }
        if (m_view->selectedSection() != 0) {
            m_view->setHighlightedDates(m_sectionMaps.at(m_view->selectedSection()));
        }

        m_view->setPreviewString(previewString);
    }

    /**
     *
     * A trick to optimize looking up selected menu item. View gives us an index
     * in the provided array of strings, we can use that index to lookup which availability
     * map for said menu item is appropriate. Alternative would getting the string and looking up
     * in the map.
     */
    static void
    updatePointersForHighlightMaps(std::vector<const utils::date::Dates *> &vec,
                                   const std::map<std::string, utils::date::Dates> &map) {
        vec.clear();
        vec.reserve(map.size());
        vec.push_back(nullptr); // 0 index = no highlighted days
        for (auto const &[_, dates] : map) {
            vec.emplace_back(&dates);
        }
    }

    /**
     * Generates a list of strings that will be displayed as menu items. Each item
     * title will be made of a key in the @p map and the number of days it has been
     * mantioned (days set).
     */
    static std::vector<std::string>
    makeMenuTitles(const std::map<std::string, utils::date::Dates> &map) {
        std::vector<std::string> strs;
        strs.reserve(map.size());
        strs.push_back(" ----- ");
        for (auto const &[str, annual_map] : map) {
            strs.push_back(std::string{"("} + std::to_string(annual_map.size()) + ") - " + str);
        }
        return std::move(strs);
    }

    auto &tagMenuItems() { return m_tagMenuItems; }
    auto &sectionMenuItems() { return m_sectionMenuItems; }
    auto &tagMaps() { return m_tagMaps; }
    auto &sectionMaps() { return m_sectionMaps; }
};

class App : public InputHandlerBase {
    std::chrono::year m_displayedYear;
    std::shared_ptr<AnnualViewBase> m_view;
    std::shared_ptr<LogRepositoryBase> m_repo;
    std::shared_ptr<EditorBase> m_editor;
    AnnualLogData m_data;

    bool m_skipFirstLine;

    std::optional<AsyncGitRepo> m_gitRepo;

    MenuListItemMaintainer m_menuListItemMaintainer;

    void
    updateViewSectionsAndTagsAfterLogChange(const std::chrono::year_month_day &dateOfChangedLog) {
        m_data.collect(m_repo, dateOfChangedLog, m_skipFirstLine);
        std::string previewString = "";
        if (dateOfChangedLog == m_view->getFocusedDate()) {
            if (auto log = m_repo->read(m_view->getFocusedDate())) {
                previewString = log->getContent();
            }
        }
        m_menuListItemMaintainer.updateViewSectionsAndTagsAfterLogChange(m_data, previewString);
    }

  public:
    App(std::shared_ptr<AnnualViewBase> view, std::shared_ptr<LogRepositoryBase> repo,
        std::shared_ptr<EditorBase> editor, bool skipFirstLine = true,
        std::optional<GitRepo> gitRepo = std::nullopt,
        std::chrono::year year = date::getToday().year())
        : m_displayedYear{year}, m_view{std::move(view)}, m_repo{std::move(repo)},
          m_editor{std::move(editor)},
          m_data{AnnualLogData::collect(m_repo, m_displayedYear, skipFirstLine)},
          m_skipFirstLine{skipFirstLine}, m_menuListItemMaintainer{m_view} {
        m_view->setInputHandler(this);
        m_view->setDatesWithLogs(&m_data.datesWithLogs);
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
        m_menuListItemMaintainer.handleFocusedTagChange(newTag);
    }

    void handleFocusedSectionChange(int newSection) {
        m_menuListItemMaintainer.handleFocusedSectionChange(newSection);
    }

    void handleUiStarted() {
        if (m_gitRepo) {
            m_view->loadingScreen("Pulling from remote...");
            m_gitRepo->pull([this] { m_view->post([this] { m_view->loadingScreenOff(); }); });
        }
    }

    void quit() {
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

    void deleteFocusedLog() {
        auto date = m_view->getFocusedDate();
        if (m_data.datesWithLogs.contains(date::monthDay(date))) {
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
        m_view->setHighlightedDates(nullptr);
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
};
} // namespace caps_log
