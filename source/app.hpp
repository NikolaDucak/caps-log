#pragma once

#include "model/log_file.hpp"
#include "model/log_repository_base.hpp"
#include "view/input_handler.hpp"
#include "view/yearly_view.hpp"

#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace clog {

using namespace view;
using namespace model;

class EditorBase {
  public:
    virtual void openEditor(const std::string &path) = 0;
};

class App : public InputHandlerBase {
    unsigned m_displayedYear;
    std::shared_ptr<YearViewBase> m_view;
    std::shared_ptr<LogRepositoryBase> m_repo;
    std::shared_ptr<EditorBase> m_editor;
    YearLogEntryData m_data;

    std::vector<const YearMap<bool> *> m_tagMaps;
    std::vector<const YearMap<bool> *> m_sectionMaps;

    void updateViewSectionsAndTagsAfterLogChange(const Date &dateOfChangedLog) {
        m_repo->injectDataForDate(m_data, dateOfChangedLog);
        updatePointersForHighlightMaps(m_tagMaps, m_data.taskMap);
        updatePointersForHighlightMaps(m_sectionMaps, m_data.sectionMap);
        m_view->setTagMenuItems(makeMenuTitles(m_data.taskMap));
        m_view->setSectionMenuItems(makeMenuTitles(m_data.sectionMap));
        if (dateOfChangedLog == m_view->getFocusedDate()) {
            if (auto log = m_repo->readLogFile(m_view->getFocusedDate())) {
                m_view->setPreviewString(log->getContent());
            } else {
                m_view->setPreviewString("");
            }
        }
    }

  public:
    App(std::shared_ptr<YearViewBase> y, std::shared_ptr<LogRepositoryBase> m,
        std::shared_ptr<EditorBase> editor)
        : m_displayedYear(Date::getToday().year), m_view{std::move(y)}, m_repo{std::move(m)},
          m_editor{std::move(editor)}, m_data{m_repo->collectDataForYear(m_displayedYear)} {
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
    bool handleRootEvent(const int input) {
        switch (input) {
        case '\x1B':
            quit();
            break;
        case 'd':
            deleteFocusedLog();
            break;
        case '+':
            displayYear(+1);
            break;
        case '-':
            displayYear(-1);
            break;
        case 'm':
            toggle();
            break;
        default:
            return false;
        };
        return true;
    }

    void handleFocusedDateChange(const int /* unused */) {
        if (auto log = m_repo->readLogFile(m_view->getFocusedDate())) {
            m_view->setPreviewString(log->getContent());
        } else {
            m_view->setPreviewString("");
        }
    }

    void handleFocusedTagChange(const int newTag) {
        m_view->selectedSection() = 0;
        const auto highlighMap = m_tagMaps.at(newTag);
        m_view->setHighlightedLogsMap(highlighMap);
    }

    void handleFocusedSectionChange(const int newSection) {
        m_view->selectedTag() = 0;
        const auto highlighMap = m_sectionMaps.at(newSection);
        m_view->setHighlightedLogsMap(highlighMap);
    }

    void quit() { m_view->stop(); }

    void deleteFocusedLog() {
        auto date = m_view->getFocusedDate();
        if (m_data.logAvailabilityMap.get(date)) {
            m_view->prompt("Are you sure you want to delete a log file?", [date, this] {
                m_repo->removeLog(date);
                updateViewSectionsAndTagsAfterLogChange(date);
            });
        }
    }

    void displayYear(int diff) {
        m_displayedYear += diff;
        m_data = m_repo->collectDataForYear(m_displayedYear);
        m_view->showCalendarForYear(m_displayedYear);
        updateViewSectionsAndTagsAfterLogChange(m_view->getFocusedDate());
    }

    void toggle() {
        /*
        const auto tag = m_view->selectedTag();
        if (tag != 0) {
            auto log = m_repo.readOrMakeLogFile(m_view->getFocusedDate());
            // if tag is available in that log, remove it
            // if (m_tagMaps.at(tag)->get(m_view->getFocusedDate())) {
            if (log.hasTag(tag)) {
                log.removeTag(tag);
            } else {
                log.addTag(tag);
            }
        }
        */
    }

    void handleCalendarButtonClick() {
        m_view->withRestoredIO([this]() {
            auto date = m_view->getFocusedDate();
            if (not m_data.logAvailabilityMap.get(date)) {
                auto log = m_repo->readLogFile(date);
                // if (log) log->write(LogFile::baseTemplate(date)
            }
            m_editor->openEditor(m_repo->path(date));

            auto log = m_repo->readLogFile(date);
            if (log && !log->hasMeaningfullContent()) {
            }
            // TODO: can sections and tasks vector be owned by controller and view only haveing
            // a refference to them? that way they can automaticaly be updated
            updateViewSectionsAndTagsAfterLogChange(date);
        });
    }

  private:
    static const YearMap<bool> *findOrNull(const StringYearMap &map, std::string key) {
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
        vec.push_back(nullptr); // 0 index = no highlighted days
        for (auto const &imap : map) {
            vec.push_back(&imap.second);
        }
    }

    /**
     * Generates a list of strings that will be displayed as menu items. Each item
     * title will be made of a key in the @p map and the number of days it has been
     * mantioned (days set).
     */
    static std::vector<std::string> makeMenuTitles(const StringYearMap &map) {
        std::vector<std::string> strs;
        strs.push_back(" ----- ");
        for (auto const &imap : map)
            strs.push_back(std::string("(") + std::to_string(imap.second.daysSet()) + ") - " +
                           imap.first);
        return std::move(strs);
    }
};
} // namespace clog
