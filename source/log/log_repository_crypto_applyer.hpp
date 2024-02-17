#pragma once

#include "utils/crypto.hpp"
#include <exception>
#include <filesystem>
#include <fstream>

#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace caps_log {

enum class Crypto { Encrypt, Decrypt };

class FileOpenError : public std::runtime_error {
  public:
    FileOpenError(const std::string &message) : std::runtime_error(message) {}
};

class FileWriteError : public std::runtime_error {
  public:
    FileWriteError(const std::string &message) : std::runtime_error(message) {}
};

class CryptoAlreadyAppliedError : public std::runtime_error {
  public:
    CryptoAlreadyAppliedError(const std::string &message) : std::runtime_error(message) {}
};

class LogRepositoryCryptoApplier {
  public:
    static constexpr auto encryptetLogRepoMarker = "encryption-marker:";
    static constexpr auto encryptetLogRepoMarkerFile = ".cle";
    static void apply(const std::string &password, const std::filesystem::path &logDirPath,
                      const std::string &logFilenameFormat, Crypto crypto);
    static bool isEncrypted(const std::filesystem::path &logDirPath);
    static bool isDecryptionPasswordValid(const std::filesystem::path &logDirPath,
                                          const std::string &password);

  private:
    LogRepositoryCryptoApplier() {}
};
} // namespace caps_log
