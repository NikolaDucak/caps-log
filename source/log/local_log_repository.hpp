#pragma once

#include "log_repository_base.hpp"
#include "utils/date.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <optional>

namespace caps_log::log {

class LocalFSLogFilePathProvider {
    std::filesystem::path m_logDirectory;
    std::string m_logFilenameFormat;

  public:
    LocalFSLogFilePathProvider(const std::filesystem::path &logDir, std::string logFilenameFormat)
        : m_logDirectory{logDir}, m_logFilenameFormat{std::move(logFilenameFormat)} {}

    [[nodiscard]] std::filesystem::path path(const std::chrono::year_month_day &date) const {
        return {m_logDirectory / fmt::format("y{}", (int)date.year()) /
                utils::date::formatToString(date, m_logFilenameFormat)};
    }

    [[nodiscard]] std::filesystem::path getLogDirPath() const { return m_logDirectory; }
    [[nodiscard]] std::string getLogFilenameFormat() const { return m_logFilenameFormat; }
};

class LocalScratchpadRepository : public ScratchpadRepositoryBase {
    std::filesystem::path m_scratchpadDirPath;
    std::string m_password;

  public:
    explicit LocalScratchpadRepository(std::filesystem::path scratchpadDirPath,
                                       std::string password = "");

    [[nodiscard]] Scratchpads read() const override;
    void remove(std::string name) override;
    void rename(std::string oldName, std::string newName) override;
};

class LocalLogRepository : public LogRepositoryBase {
    LocalFSLogFilePathProvider m_pathProvider;
    std::string m_password;

  public:
    explicit LocalLogRepository(LocalFSLogFilePathProvider pathProvider, std::string password = "");

    [[nodiscard]] std::optional<LogFile>
    read(const std::chrono::year_month_day &date) const override;
    void remove(const std::chrono::year_month_day &date) override;
    void write(const LogFile &log) override;
};

} // namespace caps_log::log
