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
#include "view/annual_view.hpp"
#include <boost/program_options.hpp>

auto md(unsigned char month, unsigned int day) {
    return std::chrono::month_day{std::chrono::month{month}, std::chrono::day{day}};
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
    const auto buttonQuit = Button(
        "Quit",
        [&]() {
            screen.Exit();
            std::exit(0);
        },
        ButtonOption::Ascii());
    const auto container = Container::Vertical({input, buttonSubmit, buttonQuit});

    const Component renderer = Renderer(container, [container]() {
        return window(text("Password required!"), container->Render()) | center;
    });

    screen.Loop(renderer);
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
    auto view = std::make_shared<view::AnnualView>(utils::date::getToday(), conf.sundayStart,
                                                   conf.recentEventsWindow);
    auto repo = std::make_shared<log::LocalLogRepository>(pathProvider, password);
    auto editor = [&]() -> std::shared_ptr<editor::EditorBase> {
        const auto envEditor = std::getenv("EDITOR");
        if (envEditor == nullptr) {
            return nullptr;
        }
        const auto editorCmd = std::string(envEditor);
        if (password.empty()) {
            return std::make_shared<editor::DefaultEditor>(pathProvider, editorCmd);
        }
        return std::make_shared<editor::EncryptedDefaultEditor>(pathProvider, password, editorCmd);
    }();
    auto gitRepo = [&]() -> std::optional<utils::GitRepo> {
        if (conf.repoConfig) {
            return std::make_optional<utils::GitRepo>(
                conf.repoConfig->root, conf.repoConfig->sshKeyPath, conf.repoConfig->sshPubKeyPath,
                conf.repoConfig->remoteName, conf.repoConfig->mainBranchName);
        }
        return std::nullopt;
    }();

    AppConfig appConf;
    appConf.skipFirstLine = conf.ignoreFirstLineWhenParsingSections;
    appConf.events = conf.calendarEvents;
    appConf.currentYear = utils::date::getToday().year();
    return caps_log::App{std::move(view), std::move(repo), std::move(editor), std::move(gitRepo),
                         std::move(appConf)};
}

void migrateToNewRepo() {
    // TODO: implement
    // if old repo has logs in root
    // for each log in root
    // parse year from title
    // create year folder if not exists
    // move log to year folder
}

int main(int argc, const char **argv) try {
    auto cliOpts = caps_log::parseCLIOptions(argc, argv);

    const auto config = caps_log::Config::make(
        [](const auto &path) { return std::make_unique<std::ifstream>(path); }, cliOpts);

    if (cliOpts.count("--encrypt") != 0U) {
        caps_log::LogRepositoryCryptoApplier::apply(config.password, config.logDirPath,
                                                    config.logFilenameFormat,
                                                    caps_log::Crypto::Encrypt);
    } else if (cliOpts.count("--decrypt") != 0U) {
        caps_log::LogRepositoryCryptoApplier::apply(config.password, config.logDirPath,
                                                    config.logFilenameFormat,
                                                    caps_log::Crypto::Decrypt);
    } else {
        makeCapsLog(config).run();
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "Captain's log encountered an error: \n " << e.what() << '\n' << std::flush;
    return 1;
}
