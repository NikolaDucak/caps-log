#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <unistd.h>
#include <utility>

#include "editor_base.hpp"
#include "log/local_log_repository.hpp"
#include "utils/crypto.hpp"

namespace caps_log::editor {

namespace detail {

inline std::string quoteShellArgument(const std::filesystem::path &path) {
    const auto value = path.string();
    std::string quoted{"'"};
    quoted.reserve(value.size() + 2);
    for (const char chr : value) {
        if (chr == '\'') {
            quoted += "'\\''";
        } else {
            quoted += chr;
        }
    }
    quoted += '\'';
    return quoted;
}

inline void openEditor(const std::string &editorCommand, const std::filesystem::path &path) {
    const auto command = fmt::format("{} {}", editorCommand, quoteShellArgument(path));
    std::ignore = std::system(command.c_str());
}

} // namespace detail

class DefaultEditor : public EditorBase {
    log::LocalFSLogFilePathProvider m_pathProvider;
    std::filesystem::path m_scratchpadDirPath;
    std::string m_editorCommand;

  public:
    explicit DefaultEditor(caps_log::log::LocalFSLogFilePathProvider pathProvider,
                           std::filesystem::path scratchpadDirPath, std::string editorCommand)
        : m_pathProvider{std::move(pathProvider)},
          m_scratchpadDirPath{std::move(scratchpadDirPath)},
          m_editorCommand{std::move(editorCommand)} {}

    void openLog(const caps_log::log::LogFile &log) override {
        detail::openEditor(m_editorCommand, m_pathProvider.path(log.getDate()));
    }

    void openScratchpad(const std::string &scratchpadName) override {
        detail::openEditor(m_editorCommand,
                           log::validatedScratchpadPath(m_scratchpadDirPath, scratchpadName));
    }
};

class EncryptedDefaultEditor : public EditorBase {
    caps_log::log::LocalFSLogFilePathProvider m_pathProvider;
    std::filesystem::path m_scratchpadPath;
    std::string m_password;
    std::string m_editorCommand;

  public:
    EncryptedDefaultEditor(caps_log::log::LocalFSLogFilePathProvider pathProvider,
                           std::filesystem::path scratchpadPath, std::string password,
                           std::string editorCommand)
        : m_pathProvider{std::move(pathProvider)}, m_scratchpadPath{std::move(scratchpadPath)},
          m_password{std::move(password)}, m_editorCommand{std::move(editorCommand)} {}

    void openLog(const caps_log::log::LogFile &log) override {
        const auto tmp = getTmpFile();
        const auto originalLogPath = m_pathProvider.path(log.getDate());
        if (std::filesystem::exists(originalLogPath)) {
            std::filesystem::copy_file(originalLogPath, tmp,
                                       std::filesystem::copy_options::overwrite_existing);
            decryptFile(tmp);
        }
        openRawEditor(tmp);
        encryptFile(tmp);
        std::filesystem::copy_file(tmp, originalLogPath,
                                   std::filesystem::copy_options::overwrite_existing);
    }

    void openScratchpad(const std::string &scratchpadName) override {
        const auto tmp = getTmpFile();
        const auto originalScratchpadPath =
            log::validatedScratchpadPath(m_scratchpadPath, scratchpadName);
        if (std::filesystem::exists(originalScratchpadPath)) {
            // Copy the original scratchpad to a temporary file
            std::filesystem::copy_file(originalScratchpadPath, tmp,
                                       std::filesystem::copy_options::overwrite_existing);
            decryptFile(tmp);
        }
        openRawEditor(tmp);
        encryptFile(tmp);
        std::filesystem::copy_file(tmp, originalScratchpadPath,
                                   std::filesystem::copy_options::overwrite_existing);
    }

  private:
    [[nodiscard]]
    static std::string getTmpFile() {
        constexpr auto kMinRandomDistribution = 100000;
        constexpr auto kMaxRandomDistribution = 999999;
        std::random_device randDevice;
        std::mt19937 gen(randDevice());
        std::uniform_int_distribution<> dis(kMinRandomDistribution, kMaxRandomDistribution);

        std::string randomFilename = "caps-log-edit-" + std::to_string(dis(gen)) + ".md";
        return (std::filesystem::temp_directory_path() / randomFilename).string();
    }

    void decryptFile(const std::string &path) {
        std::string contents;
        {
            std::ifstream source(path, std::ios::binary);
            contents = utils::decrypt(m_password, source);
        }
        {
            std::ofstream destination(path);
            destination << contents;
        }
    }

    void encryptFile(const std::string &path) {
        std::string contents;
        {
            std::ifstream source(path, std::ios::binary);
            contents = utils::encrypt(m_password, source);
        }
        {
            std::ofstream destination(path);
            destination << contents;
        }
    }

    void openRawEditor(const std::string &path) { detail::openEditor(m_editorCommand, path); }
};

} // namespace caps_log::editor
