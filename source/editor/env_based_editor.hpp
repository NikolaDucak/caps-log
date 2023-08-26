#pragma once

#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <unistd.h>
#include <utility>

#include "editor_base.hpp"
#include "model/local_log_repository.hpp"
#include "utils/crypto.hpp"

namespace caps_log::editor {

class EnvBasedEditor : public EditorBase {
    caps_log::model::LocalFSLogFilePathProvider m_pathProvider;

  public:
    EnvBasedEditor(caps_log::model::LocalFSLogFilePathProvider pathProvider)
        : m_pathProvider{std::move(pathProvider)} {}

    void openEditor(const caps_log::model::LogFile &log) override {
        if (std::getenv("EDITOR") != nullptr) {
            std::ignore = std::system(("$EDITOR " + m_pathProvider.path(log.getDate())).c_str());
        }
    }
};

class EncryptedFileEditor : public EditorBase {
    caps_log::model::LocalFSLogFilePathProvider m_pathProvider;
    std::string m_password;

  public:
    EncryptedFileEditor(caps_log::model::LocalFSLogFilePathProvider pathProvider,
                        std::string password)
        : m_pathProvider{std::move(pathProvider)}, m_password{std::move(password)} {}

    void openEditor(const caps_log::model::LogFile &log) override {
        const auto tmp = getTmpFile();
        const auto originalLogPath = m_pathProvider.path(log.getDate());
        copyLogFile(originalLogPath, tmp);
        decryptFile(tmp);
        openEnvEditor(tmp);
        encryptFile(tmp);
        copyLogFile(tmp, originalLogPath);
    }

  private:
    std::string getTmpFile() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100000, 999999);

        std::string random_filename = "caps-log-edit-" + std::to_string(dis(gen)) + ".md";
        return (std::filesystem::temp_directory_path() / random_filename).string();
    }

    void copyLogFile(const std::string &src, const std::string &dest) {
        // Implementation here...
        std::ifstream source(src, std::ios::binary);
        std::ofstream destination(dest, std::ios::binary);
        destination << source.rdbuf();
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

    void openEnvEditor(const std::string &path) {
        if (std::getenv("EDITOR") != nullptr) {
            std::ignore = std::system(("$EDITOR " + path).c_str());
        }
    }
};

} // namespace caps_log::editor
