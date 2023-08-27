#include "user_set_editor.hpp"

namespace caps_log::editor {

UserSetEditor::UserSetEditor(std::unique_ptr<EditorOpener> editorOpener,
                             model::LocalFSLogFilePathProvider pathProvider)
    : m_editorOpener{std::move(editorOpener)}, m_pathProvider{std::move(pathProvider)} {}

void UserSetEditor::openEditor(const caps_log::model::LogFile &log) {
    m_editorOpener->open(m_pathProvider.path(log.getDate()));
}

EncryptedFileUserSetEditor::EncryptedFileUserSetEditor(
    std::unique_ptr<EditorOpener> editorOpener, model::LocalFSLogFilePathProvider pathProvider,
    std::string password)
    : m_editorOpener{std::move(editorOpener)}, m_pathProvider{std::move(pathProvider)},
      m_password{std::move(password)} {}

void EncryptedFileUserSetEditor::openEditor(const caps_log::model::LogFile &log) {
    const auto tmpLogFilePath = getTmpFilePath();
    const auto originalLogPath = m_pathProvider.path(log.getDate());
    copyLogFile(originalLogPath, tmpLogFilePath);
    decryptFile(tmpLogFilePath);
    m_editorOpener->open(tmpLogFilePath);
    encryptFile(tmpLogFilePath);
    copyLogFile(tmpLogFilePath, originalLogPath);
    std::filesystem::remove(tmpLogFilePath);
}

std::string EncryptedFileUserSetEditor::getTmpFilePath() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);

    std::string random_filename = "caps-log-edit-" + std::to_string(dis(gen)) + ".md";
    return (std::filesystem::temp_directory_path() / random_filename).string();
}

void EncryptedFileUserSetEditor::copyLogFile(const std::string &src, const std::string &dest) {
    std::ifstream source(src, std::ios::binary);
    std::ofstream destination(dest, std::ios::binary);
    destination << source.rdbuf();
}

void EncryptedFileUserSetEditor::decryptFile(const std::string &path) {
    const auto contents = [&]() {
        std::ifstream source(path, std::ios::binary);
        return utils::decrypt(m_password, source);
    }();
    std::ofstream destination(path);
    destination << contents;
}

void EncryptedFileUserSetEditor::encryptFile(const std::string &path) {
    const auto contents = [&]() {
        std::ifstream source(path, std::ios::binary);
        return utils::encrypt(m_password, source);
    }();

    std::ofstream destination(path);
    destination << contents;
}

} // namespace caps_log::editor
