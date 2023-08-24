#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "app.hpp"
#include "arg_parser.hpp"
#include "config.hpp"
#include "date/date.hpp"
#include "editor/disabled_editor.hpp"
#include "editor/env_based_editor.hpp"
#include "model/local_log_repository.hpp"
#include "utils/crypto.hpp"

#include <filesystem>

auto makeClog(const clog::Config &conf) {
    using namespace clog;

    auto pathProvider = model::LocalFSLogFilePathProvider{conf.logDirPath, conf.logFilenameFormat};
    auto repo = std::make_shared<model::LocalLogRepository>(pathProvider, conf.password);
    auto view = std::make_shared<view::YearView>(date::Date::getToday(), conf.sundayStart);
    std::shared_ptr<editor::EditorBase> editor;
    if (conf.password.empty()) {
        editor = std::make_shared<editor::EnvBasedEditor>(pathProvider);
    } else {
        editor = std::make_shared<editor::EncryptedFileEditor>(pathProvider, conf.password);
    }
    return clog::App{std::move(view), std::move(repo), std::move(editor),
                     conf.ignoreFirstLineWhenParsingSections};
}

enum class Crypto { Encrypt, Decrypt };

void applyCryptograpyToLogFiles(const clog::Config &conf, Crypto c) {

    if (conf.password.empty()) {
        std::cerr << "Error: password is needed this action!";
        return;
    }

    auto logsProcessed = 0u;

    const auto tryGetLogFileStream =
        [&](const std::filesystem::directory_entry &entry) -> std::optional<std::ifstream> {
        std::tm tm;
        std::istringstream iss{entry.path().filename()};
        const auto s = std::get_time(&tm, conf.logFilenameFormat.c_str());

        if (entry.is_regular_file() && !iss.fail()) {
            std::ifstream ifs{entry.path().string()};
            if (ifs.is_open())
                return ifs;
        }
        return std::nullopt;
    };

    for (const auto &entry : std::filesystem::directory_iterator{conf.logDirPath}) {
        if (auto ifs = tryGetLogFileStream(entry)) {
            const auto fileContentsAfterCrypto = (c == Crypto::Encrypt)
                                                     ? clog::utils::encrypt(conf.password, *ifs)
                                                     : clog::utils::decrypt(conf.password, *ifs);
            if (std::ofstream ofs{entry.path().string()}; ofs.is_open()) {
                ofs << fileContentsAfterCrypto;
            } else {
                std::cerr << "Failed to write to file: " << entry.path() << std::endl;
            }
            logsProcessed++;
        } else {
            std::cerr << "Failed to open file: " << entry.path() << std::endl;
        }
    }
    std::cout << "Finished! " << ((c == Crypto::Encrypt) ? "Encrypted " : "Decrypted ")
              << logsProcessed << " log files in: " << conf.logDirPath << "." << std::endl;
}

int main(int argc, const char **argv) try {
    const auto commandLineArgs = clog::ArgParser{argc, argv};

    if (commandLineArgs.has("-h", "--help")) {
        std::cout << clog::helpString() << std::endl;
        return 0;
    }

    const auto config = clog::Config::make(
        [](const auto &path) { return std::make_unique<std::ifstream>(path); }, commandLineArgs);

    if (commandLineArgs.has("--encrypt")) {
        applyCryptograpyToLogFiles(config, Crypto::Encrypt);
    } else if (commandLineArgs.has("--decrypt")) {
        applyCryptograpyToLogFiles(config, Crypto::Decrypt);
    } else {
        makeClog(config).run();
    }

    return 0;
} catch (std::exception &e) {
    std::cerr << "clog encountered an error: \n " << e.what() << std::endl;
    return 1;
}
