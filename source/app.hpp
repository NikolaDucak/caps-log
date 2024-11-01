#pragma once

#include "editor/editor_base.hpp"
#include "log/annual_log_data.hpp"
#include "log/log_repository_base.hpp"
#include "utils/async_git_repo.hpp"
#include "view/annual_view_base.hpp"
#include "view/input_handler.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

namespace caps_log {

using namespace editor;
using namespace log;
using namespace view;
using namespace utils;

static const std::string kLogBaseTemplate{"# %d. %m. %y."};

/**
 * A helper class that updatest the view components after the data in the AnnualLogData has changed.
 * It updates the menus, the highlighted dates and the preview string, other elements of the view
 * are not part of it's set of responsibilities. This happens when the user deletes a log or adds a
 * new one or update an existing one.
 */
class ViewDataUpdater final {
    std::shared_ptr<AnnualViewBase> m_view;
    const AnnualLogData &m_data;
    std::map<std::string, MenuItems> m_tagMenuItemsPerSection;
    static constexpr auto kSelectNoneMenuEntryText = "<select none>";

  public:
    explicit ViewDataUpdater(std::shared_ptr<AnnualViewBase> view, const AnnualLogData &data);

    void handleFocusedTagChange();
    void handleFocusedSectionChange();
    void updateViewAfterDataChange(const std::string &previewString);

  private:
    void updateTagMenuItemsPerSection();

    MenuItems makeTagMenuItems(const std::vector<std::string> &tags, const std::string &section) {
        std::vector<std::string> menuItems;
        std::vector<std::string> keys;
        menuItems.reserve(tags.size());
        keys.reserve(tags.size());

        // prepend select none
        menuItems.push_back(kSelectNoneMenuEntryText);
        keys.push_back(kSelectNoneMenuEntryText);

        const auto sect =
            section == kSelectNoneMenuEntryText ? AnnualLogData::kAnySection : section;
        for (const auto &tag : tags) {
            if (tag == AnnualLogData::kAnyTag) {
                continue;
            }
            menuItems.push_back(
                makeMenuItemTitle(tag, m_data.tagsPerSection.at(sect).at(tag).size()));
            keys.push_back(tag);
        }
        return {menuItems, keys};
    }

    MenuItems makeSectionMenuItems(const std::vector<std::string> &sections) {
        std::vector<std::string> menuItems;
        std::vector<std::string> keys;
        menuItems.reserve(sections.size());
        keys.reserve(sections.size());

        // prepend select none
        menuItems.push_back(kSelectNoneMenuEntryText);
        keys.push_back(kSelectNoneMenuEntryText);
        std::cout << "section: " << sections.size() << std::endl;

        for (const auto &section : sections) {
            std::cout << "section: " << section << std::endl;
            if (section == AnnualLogData::kAnySection) {
                continue;
            }
            menuItems.push_back(makeMenuItemTitle(
                section, m_data.tagsPerSection.at(section).at(AnnualLogData::kAnyTag).size()));
            keys.push_back(section);
        }
        return {menuItems, keys};
    }
};

/**
 * The main application class that orchestrates the view, the data and the editor.
 */
class App final : public InputHandlerBase {
    std::chrono::year m_displayedYear;
    std::shared_ptr<AnnualViewBase> m_view;
    std::shared_ptr<LogRepositoryBase> m_repo;
    std::shared_ptr<EditorBase> m_editor;
    AnnualLogData m_data;
    bool m_skipFirstLine;
    std::optional<AsyncGitRepo> m_gitRepo;
    ViewDataUpdater m_viewDataUpdater;

  public:
    App(std::shared_ptr<AnnualViewBase> view, std::shared_ptr<LogRepositoryBase> repo,
        std::shared_ptr<EditorBase> editor, bool skipFirstLine = true,
        std::optional<GitRepo> gitRepo = std::nullopt,
        std::chrono::year year = date::getToday().year());

    void run();
    bool handleInputEvent(const UIEvent &event) override;

  private:
    bool handleRootEvent(const std::string &input);
    void handleFocusedDateChange();
    void handleFocusedTagChange();
    void handleFocusedSectionChange();
    void handleUiStarted();
    void handleDisplayedYearChange(int diff);
    void handleCalendarButtonClick();

    void updateDataAndViewAfterLogChange(const std::chrono::year_month_day &dateOfChangedLog);
    void deleteFocusedLog();
    void quit();

    static bool noMeaningfullContent(const std::string &content,
                                     const std::chrono::year_month_day &date);
};
} // namespace caps_log
