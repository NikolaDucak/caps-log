#include "log_repository_crypto_applier.hpp"
#include "utils/crypto.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

namespace caps_log {

namespace {
void updateEncryptionMarkerfile(Crypto crypto, const std::filesystem::path &logDirPath,
                                const std::string &password) {
    const auto markerFilePath =
        logDirPath / LogRepositoryCryptoApplier::kEncryptedLogRepoMarkerFile;
    if (crypto == Crypto::Encrypt) {
        std::ofstream cle{markerFilePath, std::ios::binary};
        if (not cle.is_open()) {
            // This would be an unfortunate situation, the repo has already been encrypted
            // but the encryption marker file is not added.
            // TODO: figure something out
            throw std::runtime_error{"Failed writing encryption marker file"};
        }
        std::istringstream oss{LogRepositoryCryptoApplier::kEncryptedLogRepoMarker};
        cle << utils::encrypt(password, oss);
    }
    if (crypto == Crypto::Decrypt) {
        std::filesystem::remove(markerFilePath);
    }
}

std::optional<std::ifstream> openInputFileStream(const std::filesystem::path &path) {
    std::ifstream ifs(path, std::ifstream::in);
    if (!ifs.is_open()) {
        return std::nullopt;
    }
    return std::move(ifs);
}

std::optional<std::ofstream> openOutputFileStream(const std::filesystem::path &path) {
    std::ofstream ofs(path, std::ofstream::trunc);
    if (!ofs.is_open()) {
        return std::nullopt;
    }
    return std::move(ofs);
}

bool fileMatchesLogFilenameFormat(const std::filesystem::directory_entry &entry,
                                  const std::string &logFilenameFormat) {
    std::tm time{};
    std::istringstream iss(entry.path().filename().string());
    iss >> std::get_time(&time, logFilenameFormat.c_str());
    // if it does not match the pattern, return false
    if (iss.fail()) {
        return false;
    }

    // if there is something after the name that matches the pattern, return false
    std::string remaining;
    std::getline(iss, remaining);
    return remaining.empty();
}

bool cryptoAlreadyApplied(const std::filesystem::path &logDirPath, Crypto crypto) {
    bool encryptionMarkerfilePresent = std::filesystem::exists((logDirPath / ".cle"));
    if (crypto == Crypto::Encrypt) {
        return encryptionMarkerfilePresent;
    }
    if (crypto == Crypto::Decrypt) {
        return !encryptionMarkerfilePresent;
    }
    throw std::runtime_error{"Unreachable"};
}
} // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void LogRepositoryCryptoApplier::apply(const std::string &password,
                                       const std::filesystem::path &logDirPath,
                                       const std::string &logFilenameFormat, Crypto crypto) {
    if (cryptoAlreadyApplied(logDirPath, crypto)) {
        throw CryptoAlreadyAppliedError{"Already applied"};
    }

    if (password.empty()) {
        throw std::invalid_argument("Error: password is needed for this action!");
    }

    std::vector<std::exception> errors; // Accumulate errors here

    // Function to process individual log files
    const auto processLogFile = [&](const std::filesystem::directory_entry &entry) {
        if (auto ifsOpt = openInputFileStream(entry.path())) {
            std::istream &ifs = *ifsOpt;
            std::string fileContentsAfterCrypto = (crypto == Crypto::Encrypt)
                                                      ? utils::encrypt(password, ifs)
                                                      : utils::decrypt(password, ifs);
            if (auto ofsOpt = openOutputFileStream(entry.path())) {
                std::ostream &ofs = *ofsOpt;
                ofs << fileContentsAfterCrypto;
            } else {
                errors.push_back(
                    FileWriteError("Failed to write to file: " + entry.path().string()));
            }
        } else {
            errors.push_back(FileOpenError("Failed to open file: " + entry.path().string()));
        }
    };

    const auto isYearDir = [&](const std::filesystem::directory_entry &entry) {
        const auto filename = entry.path().filename().string();
        static constexpr auto kNumCharsForYearDirName = 5; /* yYYYY */
        return entry.is_directory() && filename.size() == kNumCharsForYearDirName &&
               filename[0] == 'y' &&
               std::all_of(filename.begin() + 1, filename.end(),
                           [](char chr) { return std::isdigit(chr); });
    };

    // Iterate over all files in the directory
    for (const auto &entry : std::filesystem::directory_iterator{logDirPath}) {
        if (!isYearDir(entry)) {
            continue;
        }
        for (const auto &subEntry : std::filesystem::directory_iterator{entry.path()}) {
            if (!(subEntry.is_regular_file() &&
                  fileMatchesLogFilenameFormat(subEntry, logFilenameFormat))) {
                continue;
            }
            try {
                processLogFile(subEntry);
            } catch (const std::exception &e) {
                // Do not throw here, accumulate errors instead
                errors.push_back(e);
            }
        }
    }

    // Check if there are any errors, and if so, throw a combined exception
    if (!errors.empty()) {
        std::string errorMessage = "Error(s) occurred during file processing:";
        std::string delimiter = "\n - ";

        for (const auto &error : errors) {
            errorMessage += delimiter + error.what();
        }
        throw std::runtime_error(errorMessage);
    }

    updateEncryptionMarkerfile(crypto, logDirPath, password);
}

bool LogRepositoryCryptoApplier::isEncrypted(const std::filesystem::path &logDirPath) {
    bool encryptionMarkerfilePresent = std::filesystem::exists(
        logDirPath / LogRepositoryCryptoApplier::kEncryptedLogRepoMarkerFile);
    return encryptionMarkerfilePresent;
}

bool LogRepositoryCryptoApplier::isDecryptionPasswordValid(const std::filesystem::path &logDirPath,
                                                           const std::string &password) {
    const auto encryptionMarkerfile =
        logDirPath / LogRepositoryCryptoApplier::kEncryptedLogRepoMarkerFile;
    auto cleStream = std::ifstream{encryptionMarkerfile};
    auto decryptedMarker = utils::decrypt(password, cleStream);
    return decryptedMarker.find(LogRepositoryCryptoApplier::kEncryptedLogRepoMarker) == 0;
}

} // namespace caps_log
