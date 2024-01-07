#include <algorithm>
#include <filesystem>
#include <fstream>
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

auto makeCapsLog(const caps_log::Config &conf) {
    using namespace caps_log;

    auto pathProvider = log::LocalFSLogFilePathProvider{conf.logDirPath, conf.logFilenameFormat};
    auto view = std::make_shared<view::AnnualView>(date::Date::getToday(), conf.sundayStart);
    auto repo = std::make_shared<log::LocalLogRepository>(pathProvider, conf.password);
    auto editor = [&]() -> std::shared_ptr<editor::EditorBase> {
        if (conf.password.empty()) {
            return std::make_shared<editor::EnvBasedEditor>(pathProvider);
        }
        return std::make_shared<editor::EncryptedFileEditor>(pathProvider, conf.password);
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
        caps_log::LogRepositoryCrypoApplier::apply(config.password, config.logDirPath,
                                                   config.logFilenameFormat,
                                                   caps_log::Crypto::Encrypt);
    } else if (cliOpts.count("--decrypt") != 0U) {
        caps_log::LogRepositoryCrypoApplier::apply(config.password, config.logDirPath,
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
