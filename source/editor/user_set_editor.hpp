#pragma once

#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <unistd.h>
#include <utility>

#include "editor_base.hpp"
#include "editor_opener.hpp"
#include "model/local_log_repository.hpp"
#include "utils/crypto.hpp"

namespace caps_log::editor {

class UserSetEditor : public EditorBase {
    std::unique_ptr<EditorOpener> m_editorOpener;
    model::LocalFSLogFilePathProvider m_pathProvider;

  public:
    UserSetEditor(std::unique_ptr<EditorOpener> editorOpener,
                  model::LocalFSLogFilePathProvider pathProvider);

    void openEditor(const caps_log::model::LogFile &log) override;
};

class EncryptedFileUserSetEditor : public EditorBase {
    std::unique_ptr<EditorOpener> m_editorOpener;
    model::LocalFSLogFilePathProvider m_pathProvider;
    std::string m_password;

  public:
    EncryptedFileUserSetEditor(std::unique_ptr<EditorOpener> editorOpener,
                               model::LocalFSLogFilePathProvider pathProvider,
                               std::string password);

    void openEditor(const caps_log::model::LogFile &log) override;

  private:
    std::string getTmpFilePath();
    void copyLogFile(const std::string &src, const std::string &dest);
    void decryptFile(const std::string &path);
    void encryptFile(const std::string &path);
};

} // namespace caps_log::editor
