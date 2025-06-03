#include <fmt/format.h>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <iostream>
#include <optional>
#include <string>

#include "app.hpp"
#include "config.hpp"
#include "editor/default_editor.hpp"
#include "log/local_log_repository.hpp"
#include "log/log_repository_crypto_applier.hpp"
#include "utils/git_repo.hpp"
#include <boost/program_options.hpp>

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
void migrateToNewRepoStructureIfNeeded(
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
std::string promptPassword(const caps_log::Config &config) {
    using namespace ftxui;
    auto screen = ScreenInteractive::Fullscreen();
    std::string password;
    const auto input = Input(InputOption{.content = &password,
                                         .password = true,
                                         .multiline = false,
                                         .on_enter = screen.ExitLoopClosure()});
    const auto buttonSubmit = Button("Submit", screen.ExitLoopClosure(), ButtonOption::Ascii());
    bool shouldExit = false;
    const auto buttonQuit = Button(
        "Quit",
        [&]() {
            shouldExit = true;
            screen.Exit();
        },
        ButtonOption::Ascii());
    const auto container = Container::Vertical({input, buttonSubmit, buttonQuit});

    const Component renderer = Renderer(container, [container]() {
        return window(text("Password required!"), container->Render()) | center;
    });

    screen.Loop(renderer);
    if (shouldExit) {
        std::exit(0);
    }
    return password;
}

std::string getPasswordFromTUIIfNeededAndNotProvided(const caps_log::Config &conf) {
    std::string pass = conf.password;
    if (caps_log::LogRepositoryCryptoApplier::isEncrypted(conf.logDirPath) && pass.empty()) {
        pass = promptPassword(conf);
    }
    return pass;
}

auto makeCapsLog(const caps_log::Config &conf) {
    using namespace caps_log;
    const auto password = getPasswordFromTUIIfNeededAndNotProvided(conf);
    auto pathProvider = log::LocalFSLogFilePathProvider{conf.logDirPath, conf.logFilenameFormat};
    auto view = std::make_shared<view::View>(
        view::ViewConfig{.today = utils::date::getToday(),
                         .sundayStart = conf.sundayStart,
                         .recentEventsWindow = conf.recentEventsWindow});
    auto repo = std::make_shared<log::LocalLogRepository>(pathProvider, password);
    auto scratchpadRepo = std::make_shared<log::LocalScratchpadRepository>(
        conf.logDirPath / Config::kDefaultScratchpadFolderName, password);
    auto editor = [&]() -> std::shared_ptr<editor::EditorBase> {
        auto *const envEditor = std::getenv("EDITOR");
        if (envEditor == nullptr) {
            return nullptr;
        }
        const auto editorCmd = std::string(envEditor);
        if (password.empty()) {
            return std::make_shared<editor::DefaultEditor>(
                pathProvider, conf.logDirPath / Config::kDefaultScratchpadFolderName, editorCmd);
        }
        return std::make_shared<editor::EncryptedDefaultEditor>(
            pathProvider, conf.logDirPath / Config::kDefaultScratchpadFolderName, password,
            editorCmd);
    }();
    auto gitRepo = [&]() -> std::optional<utils::GitRepo> {
        if (conf.repoConfig) {
            return std::make_optional<utils::GitRepo>(
                conf.repoConfig->root, conf.repoConfig->sshKeyPath, conf.repoConfig->sshPubKeyPath,
                conf.repoConfig->remoteName, conf.repoConfig->mainBranchName);
        }
        return std::nullopt;
    }();

    migrateToNewRepoStructureIfNeeded(pathProvider);

    AppConfig appConf{
        .skipFirstLine = conf.ignoreFirstLineWhenParsingSections,
        .currentYear = utils::date::getToday().year(),
        .events = conf.calendarEvents,
    };

    return caps_log::App{std::move(view),   std::move(repo),    std::move(scratchpadRepo),
                         std::move(editor), std::move(gitRepo), std::move(appConf)};
}

int main(int argc, const char **argv) try {
    auto cliOpts = caps_log::parseCLIOptions(std::span(argv, argc));

    const auto config = caps_log::Config::make(
        [](const auto &path) { return std::make_unique<std::ifstream>(path); }, cliOpts);

    if (cliOpts.contains("--encrypt")) {
        caps_log::LogRepositoryCryptoApplier::apply(
            config.password, config.logDirPath, caps_log::Config::kDefaultScratchpadFolderName,
            config.logFilenameFormat, caps_log::Crypto::Encrypt);
    } else if (cliOpts.contains("--decrypt")) {
        caps_log::LogRepositoryCryptoApplier::apply(
            config.password, config.logDirPath, caps_log::Config::kDefaultScratchpadFolderName,
            config.logFilenameFormat, caps_log::Crypto::Decrypt);
    } else {
        makeCapsLog(config).run();
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "Captain's log encountered an error: \n " << e.what() << '\n' << std::flush;
    return 1;
}
