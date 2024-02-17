#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "app.hpp"
#include "config.hpp"
#include "date/date.hpp"
#include "editor/disabled_editor.hpp"
#include "editor/env_based_editor.hpp"
#include "log/local_log_repository.hpp"
#include "log/log_repository_crypto_applyer.hpp"
#include "utils/crypto.hpp"
#include "utils/git_repo.hpp"
#include "version.hpp"
#include <boost/program_options.hpp>

std::string promptPassword(const caps_log::Config &config) {
    using namespace ftxui;
    auto screen = ScreenInteractive::Fullscreen();
    std::string password;
    const auto input = Input(InputOption{.content = &password,
                                         .password = true,
                                         .multiline = false,
                                         .on_enter = screen.ExitLoopClosure()});
    const auto button_submit = Button("Submit", screen.ExitLoopClosure(), ButtonOption::Ascii());
    const auto button_quit = Button("Quit", screen.ExitLoopClosure(), ButtonOption::Ascii());
    const auto container = Container::Vertical({input, button_submit, button_quit});

    const Component renderer = Renderer(container, [container]() {
        return window(text("Pasword required!"), container->Render()) | center;
    });

    screen.Loop(renderer);
    return password;
}

std::string getPassword(const caps_log::Config &conf) {
    std::string pass = conf.password;
    if (caps_log::LogRepositoryCryptoApplier::isEncrypted(conf.logDirPath) && pass.empty()) {
        pass = promptPassword(conf);
    }
    return pass;
}

auto makeCapsLog(const caps_log::Config &conf) {
    using namespace caps_log;
    const auto pass = getPassword(conf);
    auto pathProvider = log::LocalFSLogFilePathProvider{conf.logDirPath, conf.logFilenameFormat};
    auto view = std::make_shared<view::AnnualView>(date::Date::getToday(), conf.sundayStart);
    auto repo = std::make_shared<log::LocalLogRepository>(pathProvider, pass);
    auto editor = [&]() -> std::shared_ptr<editor::EditorBase> {
        if (pass.empty()) {
            return std::make_shared<editor::EnvBasedEditor>(pathProvider);
        }
        return std::make_shared<editor::EncryptedFileEditor>(pathProvider, pass);
    }();
    auto gitRepo = [&]() -> std::optional<utils::GitRepo> {
        if (conf.repoConfig) {
            return std::make_optional<utils::GitRepo>(
                conf.repoConfig->root, conf.repoConfig->sshKeyPath, conf.repoConfig->sshPubKeyPath,
                conf.repoConfig->remoteName, conf.repoConfig->mainBranchName);
        }
        return std::nullopt;
    }();

    return caps_log::App{std::move(view), std::move(repo), std::move(editor),
                         conf.ignoreFirstLineWhenParsingSections, std::move(*gitRepo)};
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
    std::cerr << "Captains log encountered an error: \n " << e.what() << std::endl;
    return 1;
}
