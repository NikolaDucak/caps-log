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
#include "version.hpp"
#include <boost/program_options.hpp>

auto makeCapsLog(const caps_log::Config &conf) {
    using namespace caps_log;

    auto pathProvider = log::LocalFSLogFilePathProvider{conf.logDirPath, conf.logFilenameFormat};
    auto repo = std::make_shared<log::LocalLogRepository>(pathProvider, conf.password);
    auto view = std::make_shared<view::YearView>(date::Date::getToday(), conf.sundayStart);
    std::shared_ptr<editor::EditorBase> editor;
    if (conf.password.empty()) {
        editor = std::make_shared<editor::EnvBasedEditor>(pathProvider);
    } else {
        editor = std::make_shared<editor::EncryptedFileEditor>(pathProvider, conf.password);
    }
    return caps_log::App{std::move(view), std::move(repo), std::move(editor),
                         conf.ignoreFirstLineWhenParsingSections};
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
} catch (std::exception &e) {
    std::cerr << "Captains log encountered an error: \n " << e.what() << std::endl;
    return 1;
}
