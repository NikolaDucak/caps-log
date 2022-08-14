#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "app.hpp"
#include "arg_parser.hpp"
#include "config.hpp"
#include "date/date.hpp"
#include "editor/disabled_editor.hpp"
#include "editor/env_based_editor.hpp"
#include "model/local_log_repository.hpp"

auto makeClog(const clog::Config &conf) {
    using namespace clog;

    auto pathProvider = model::LocalFSLogFilePathProvider{conf.logDirPath, conf.logFilenameFormat};
    auto repo = std::make_shared<model::LocalLogRepository>(pathProvider);
    auto view = std::make_shared<view::YearView>(date::Date::getToday(), conf.sundayStart);
    auto editor = std::make_shared<editor::EnvBasedEditor>(pathProvider);
    return clog::App{std::move(view), std::move(repo), std::move(editor),
                     conf.ignoreFirstLineWhenParsingSections};
}

int main(int argc, const char **argv) try {
    const auto commandLineArgs = clog::ArgParser{argc, argv};

    if (commandLineArgs.has("-h", "--help")) {
        std::cout << clog::helpString() << std::endl;
    } else {
        auto config = clog::Config::make(
            [](const auto &path) { return std::make_unique<std::ifstream>(path); },
            commandLineArgs);
        makeClog(config).run();
    }

    return 0;
} catch (std::exception &e) {
    std::cout << "clog encountered an error: \n " << e.what() << std::endl;
    return 1;
}
