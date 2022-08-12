#pragma once

#include "editor/editor_base.hpp"
#include "model/log_file.hpp"
#include "model/log_repository_base.hpp"
#include "model/year_overview_data.hpp"
#include "utils/string.hpp"
#include "view/input_handler.hpp"
#include "view/yearly_view.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace clog {

using namespace view;
using namespace model;
using namespace editor;

inline static std::string LOG_BASE_TEMPLATE{"# %d. %m. %y."};

class App : public InputHandlerBase {
    unsigned m_displayedYear;
    std::shared_ptr<YearViewBase> m_view;
    std::shared_ptr<LogRepositoryBase> m_repo;
    std::shared_ptr<EditorBase> m_editor;
    YearOverviewData m_data;

    std::vector<const YearMap<bool> *> m_tagMaps;
    std::vector<const YearMap<bool> *> m_sectionMaps;
    bool m_skipFirstLine;

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
    App(std::shared_ptr<YearViewBase> view, std::shared_ptr<LogRepositoryBase> repo,
        std::shared_ptr<EditorBase> editor, bool skipFirstLine = true)
        : m_displayedYear(Date::getToday().year), m_view{std::move(view)}, m_repo{std::move(repo)},
          m_editor{std::move(editor)}, m_data{YearOverviewData::collect(
                                           m_repo, date::Date::getToday().year, skipFirstLine)},
          m_skipFirstLine{skipFirstLine} {
        m_view->setInputHandler(this);
        m_view->setAvailableLogsMap(&m_data.logAvailabilityMap);
        updateViewSectionsAndTagsAfterLogChange(m_view->getFocusedDate());
    }

    void run() { m_view->run(); }

    bool handleInputEvent(const UIEvent &event) override {
        switch (event.type) {
        case UIEvent::ROOT_EVENT:
            return handleRootEvent(event.input);
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
        } else if (input == "m") {
            toggle();
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

    void quit() { m_view->stop(); }

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
        m_data = YearOverviewData::collect(m_repo, m_displayedYear, m_skipFirstLine);
        m_view->showCalendarForYear(m_displayedYear);
        updateViewSectionsAndTagsAfterLogChange(m_view->getFocusedDate());
    }

    void toggle() {
        /*
        const auto tag = m_view->selectedTag();
        if (tag != 0) {
            auto log = m_repo.read(m_view->getFocusedDate());
            if (log) {
                // if (m_tagMaps.at(tag)->get(m_view->getFocusedDate())) {
                if (log.hasTag(tag)) {
                    log.removeTag(tag);
                } else {
                    log.addTag(tag);
                }

            } else {
                m_repo.write(makeLogEntryWithTag(tag))
            }
        }
        */
    }

    // TODO: i feel like this needs more tests
    // 2) editor removes log
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
            if (log && noMeningfullContent(log->getContent(), date)) {
                m_repo->remove(date);
            }
            updateViewSectionsAndTagsAfterLogChange(date);
        });
    }

    static bool noMeningfullContent(const std::string &content, const Date &date) {
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
} // namespace clog
