#pragma once

#include <filesystem>
#include <string>

namespace caps_log {

enum class Crypto : std::uint8_t { Encrypt, Decrypt };

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
    static constexpr auto kEncryptedLogRepoMarker = "encryption-marker:";
    static constexpr auto kEncryptedLogRepoMarkerFile = ".cle";
    static void apply(const std::string &password, const std::filesystem::path &logDirPath,
                      const std::string &logFilenameFormat, Crypto crypto);
    [[nodiscard]] static bool isEncrypted(const std::filesystem::path &logDirPath);
    [[nodiscard]] static bool isDecryptionPasswordValid(const std::filesystem::path &logDirPath,
                                                        const std::string &password);

  private:
    LogRepositoryCryptoApplier() {}
};
} // namespace caps_log
