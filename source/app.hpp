#pragma once

#include "editor/editor_base.hpp"
#include "log/annual_log_data.hpp"
#include "log/log_repository_base.hpp"
#include "utils/async_git_repo.hpp"
#include "view/annual_view_layout_base.hpp"
#include "view/input_handler.hpp"
#include "view/view.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>

namespace caps_log {

const std::string kLogBaseTemplate{"# %d. %m. %y."};

/**
 * A helper class that updates the view components after the data in the AnnualLogData has changed.
 * It updates the menus, the highlighted dates and the preview string, other elements of the view
 * are not part of it's set of responsibilities. This happens when the user deletes a log or adds a
 * new one or update an existing one.
 */
class ViewDataUpdater final {
    std::shared_ptr<view::AnnualViewLayoutBase> m_view;
    const log::AnnualLogData &m_data; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    std::map<std::string, view::MenuItems> m_tagMenuItemsPerSection;
    static constexpr auto kSelectNoneMenuEntryText = "<select none>";

  public:
    explicit ViewDataUpdater(std::shared_ptr<view::AnnualViewLayoutBase> view,
                             const log::AnnualLogData &data);

    void handleFocusedTagChange();
    void handleFocusedSectionChange();
    void updateViewAfterDataChange(const std::string &previewTitle,
                                   const std::string &previewString);

  private:
    void updateTagMenuItemsPerSection();

    view::MenuItems makeTagMenuItems(const std::string &section);
    view::MenuItems makeSectionMenuItems();
};

struct AppConfig {
  public:
    bool skipFirstLine;
    std::chrono::year currentYear;
    view::CalendarEvents events;
};

/**
 * The main application class that orchestrates the view, the data and the editor.
 */
class App final : public view::InputHandlerBase {
    AppConfig m_config;
    std::shared_ptr<view::ViewBase> m_view;
    std::shared_ptr<log::LogRepositoryBase> m_repo{nullptr};
    std::shared_ptr<log::ScratchpadRepositoryBase> m_scratchpadRepo{nullptr};
    std::shared_ptr<editor::EditorBase> m_editor;
    log::AnnualLogData m_data;
    std::optional<utils::AsyncGitRepo> m_gitRepo;
    ViewDataUpdater m_viewDataUpdater;

    struct AskForPassword {
        std::function<std::shared_ptr<log::LogRepositoryBase>(std::string)> logRepoFactory;
        std::function<std::shared_ptr<log::ScratchpadRepositoryBase>(std::string)>
            scratchpadRepoFactory;
        std::function<std::shared_ptr<editor::EditorBase>(std::string)> editorFactory;
    };

    std::optional<AskForPassword> m_askForPassword;

  public:
    /**
     * Constructs the App with the given view, log repository, scratchpad repository, editor,
     * optional git repository and configuration.
     * @note The log and scratchpad repositories are expected to be already
     * configured with the necessary password if encryption is used.
     */
    App(std::shared_ptr<view::ViewBase> view, std::shared_ptr<log::LogRepositoryBase> repo,
        std::shared_ptr<log::ScratchpadRepositoryBase> scratchpadRepo,
        std::shared_ptr<editor::EditorBase> editor,
        std::optional<utils::GitRepo> gitRepo = std::nullopt, AppConfig config = {});

    /**
     * Constructs the App with the given view, log repository factory, scratchpad repository
     * factory, editor, optional git repository and configuration. The factories are used
     * to create the repositories once the password is provided by the user.
     * @note This constructor is used when the log files are encrypted and the password
     * needs to be obtained from the user at runtime.
     */
    App(std::shared_ptr<view::ViewBase> view,
        std::function<std::shared_ptr<log::LogRepositoryBase>(std::string)> aLogRepoFactory,
        std::function<std::shared_ptr<log::ScratchpadRepositoryBase>(std::string)>
            aScratchpadRepoFactory,
        std::function<std::shared_ptr<editor::EditorBase>(std::string)> aEditorFactory,
        std::optional<utils::GitRepo> gitRepo, AppConfig config);

    App(const App &) = delete;
    App(App &&) = delete;
    App &operator=(const App &) = delete;
    App &operator=(App &&) = delete;
    ~App() override { quit(); }

    void run();
    bool handleInputEvent(const view::UIEvent &event) override;

  private:
    bool handleRootEvent(const std::string &input);
    void handleFocusedDateChange();
    void handleFocusedTagChange();
    void handleFocusedSectionChange();
    void handleUiStarted();
    void handleDisplayedYearChange(int diff);
    void handleOpenScratchpad(std::string name);
    void handleDeleteScratchpad(std::string name);
    void handleRenameScratchpad(std::string name);
    void handleOpenLogFile();
    void handleSwitchLayout();

    void updateDataAndViewAfterLogChange(const std::chrono::year_month_day &dateOfChangedLog);
    void deleteFocusedLog();
    void quit();
};
} // namespace caps_log
