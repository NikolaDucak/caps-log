#pragma once

#include "editor/default_editor.hpp"
#include "editor/editor_base.hpp"
#include "log/log_repository_crypto_applier.hpp"
#include "utils/git_repo.hpp"
#include <app.hpp>
#include <boost/property_tree/ptree.hpp>
#include <config.hpp>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/screen/terminal.hpp>
#include <functional>
#include <log/local_log_repository.hpp>
#include <string>
#include <utility>
#include <view/view.hpp>

namespace caps_log {

using EditorFactory = std::function<std::shared_ptr<editor::EditorBase>(
    const log::LocalFSLogFilePathProvider &, const std::string &, const std::string &)>;

class CapsLog {
  public:
    struct Context {
        std::chrono::year_month_day today;
        std::vector<std::string> cliArgs;
        EditorFactory editorFactory = defaultEditorFactory;
        std::function<ftxui::Dimensions()> terminalSizeProvider = defaultScreenSizeProvider;
    };

    struct Task {
        enum class Type : char { kRunAppplication, kApplyCrypto, kInvalidCliArgs, kInvalidConfig };
        Type type;
        std::function<void()> action;
    };

    CapsLog(const Context &context) : m_config(context.cliArgs) {
        m_config.verify();

        if (!m_config.shouldRunApplication()) {
            return;
        }

        m_view = std::make_shared<view::View>(m_config.getViewConfig(), context.today,
                                              context.terminalSizeProvider);

        auto gitRepo = [this] -> std::optional<utils::GitRepo> {
            if (m_config.getGitRepoConfig()) {
                return std::make_optional<utils::GitRepo>(*m_config.getGitRepoConfig());
            }
            return std::nullopt;
        }();

        // TODO: unify hanling of injected todays date
        auto conf = m_config.getAppConfig();
        conf.currentYear = context.today.year();

        const auto shouldAskForPassword =
            LogRepositoryCryptoApplier::isEncrypted(m_config.getLogDirPath()) &&
            !m_config.isPasswordProvided();

        if (shouldAskForPassword) {
            auto logRepoFactory = [pathProvider = m_config.getLogFilePathProvider(),
                                   password = m_config.getPassword()](const std::string &pwd) {
                return std::make_shared<log::LocalLogRepository>(pathProvider, pwd);
            };
            auto scratchpadRepoFactory =
                [scratchpadDirPath = m_config.getScratchpadDirPath()](const std::string &pwd) {
                    return std::make_shared<log::LocalScratchpadRepository>(scratchpadDirPath, pwd);
                };
            auto editorFactory = [this, factory = context.editorFactory](const std::string &pwd) {
                return factory(m_config.getLogFilePathProvider(), m_config.getScratchpadDirPath(),
                               pwd);
            };

            m_app = std::make_shared<App>(m_view, std::move(logRepoFactory),
                                          std::move(scratchpadRepoFactory),
                                          std::move(editorFactory), std::move(gitRepo), conf);
        } else {
            auto logRepo = std::make_shared<log::LocalLogRepository>(
                m_config.getLogFilePathProvider(), m_config.getPassword());
            auto scratchpadRepo = std::make_shared<log::LocalScratchpadRepository>(
                m_config.getScratchpadDirPath(), m_config.getPassword());
            auto editor =
                context.editorFactory(m_config.getLogFilePathProvider(),
                                      m_config.getScratchpadDirPath(), m_config.getPassword());

            m_app = std::make_shared<App>(m_view, logRepo, scratchpadRepo, editor,
                                          std::move(gitRepo), conf);
        }
    }

    [[nodiscard]] Task getTask() {
        if (m_config.shouldRunApplication()) {
            return Task{Task::Type::kRunAppplication, [this]() { run(); }};
        }
        if (m_config.getCryptoApplicationType()) {
            return Task{Task::Type::kApplyCrypto,
                        [this]() { applyCrypto(*m_config.getCryptoApplicationType()); }};
        }
        return Task{Task::Type::kInvalidCliArgs, []() {}};
    }

    // used for testing purposes
    bool onEvent(ftxui::Event event) { return m_view->onEvent(std::move(event)); }

    // used for testing purposes
    [[nodiscard]] std::string render() const { return m_view->render(); }

    // used for testing purposes
    [[nodiscard]] const Configuration &getConfig() const { return m_config; }

  private:
    void run() {
        if (m_app) {
            m_app->run();
        } else {
            throw std::runtime_error("Application is not initialized.");
        }
    }

    void applyCrypto(Crypto crypto) {
        caps_log::LogRepositoryCryptoApplier::apply(
            m_config.getPassword(), m_config.getLogDirPath(),
            caps_log::Configuration::kDefaultScratchpadFolderName, m_config.getLogFilenameFormat(),
            crypto);
    }

    /**
     * If the curret log dir has a structure like
     * /
     *   - 2021-01-01.md
     *   - 2021-01-02.md
     *   - ..
     *   - 2024-12-31.md
     * then migrate to
     * /
     *   - 2021/
     *     - 2021-01-01.md
     *     - 2021-01-02.md
     *   - 2022/
     *    - 2022-01-01.md
     */
    static void migrateToNewRepoStructureIfNeeded(
        const caps_log::log::LocalFSLogFilePathProvider &pathProvider) {

        const auto logDirPath = pathProvider.getLogDirPath();
        const auto logFilenameFormat = pathProvider.getLogFilenameFormat();

        const auto parseDate =
            [logFilenameFormat](
                const std::filesystem::path &path) -> std::optional<std::chrono::year_month_day> {
            const auto filenameStr = path.filename().string();
            std::tm dateTime = {};
            std::istringstream iss{filenameStr};
            if (!(iss >> std::get_time(&dateTime, logFilenameFormat.c_str()))) {
                return std::nullopt;
            }

            std::string remaining;
            std::getline(iss, remaining);
            if (!remaining.empty()) {
                return std::nullopt;
            }
            // The tm_year is years since 1900, so we need to add 1900 to it
            static constexpr auto kTmYearOffset = 1900;
            return std::chrono::year_month_day{
                std::chrono::year{dateTime.tm_year + kTmYearOffset},
                std::chrono::month{static_cast<unsigned int>(dateTime.tm_mon + 1)},
                std::chrono::day{static_cast<unsigned int>(dateTime.tm_mday)}};
        };

        for (const auto &entry : std::filesystem::directory_iterator{logDirPath}) {
            const auto date = parseDate(entry.path());
            if (!date.has_value()) {
                continue;
            }
            const auto newPath = pathProvider.path(date.value());
            if (entry.path() == newPath) {
                throw std::runtime_error{"Log file already in correct location!"};
            }
            if (std::filesystem::exists(newPath)) {
                throw std::runtime_error{
                    fmt::format("Log file already exists at: {}", newPath.string())};
            }
            std::filesystem::create_directories(newPath.parent_path());
            std::filesystem::rename(entry.path(), newPath);
        }
    }

    Configuration m_config;
    std::shared_ptr<view::View> m_view;
    std::shared_ptr<App> m_app;

    static std::shared_ptr<editor::EditorBase>
    defaultEditorFactory(const log::LocalFSLogFilePathProvider &logFilePath,
                         const std::string &scratchpadDirPath, const std::string &password) {
        auto *editorCommand = std::getenv("EDITOR");
        if (editorCommand == nullptr) {
            return nullptr;
        }
        if (not password.empty()) {
            return std::make_shared<editor::EncryptedDefaultEditor>(
                logFilePath, scratchpadDirPath, password, std::string{editorCommand});
        }
        return std::make_shared<editor::DefaultEditor>(logFilePath, scratchpadDirPath,
                                                       std::string{editorCommand});
    }

    static ftxui::Dimensions defaultScreenSizeProvider() { return ftxui::Terminal::Size(); }
};

} // namespace caps_log
