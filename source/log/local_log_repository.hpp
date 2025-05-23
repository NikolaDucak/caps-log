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

    [[nodiscard]] inline std::filesystem::path path(const std::chrono::year_month_day &date) const {
        return {m_logDirectory / fmt::format("y{}", (int)date.year()) /
                utils::date::formatToString(date, m_logFilenameFormat)};
    }

    [[nodiscard]] inline std::filesystem::path getLogDirPath() const { return m_logDirectory; }
    [[nodiscard]] inline std::string getLogFilenameFormat() const { return m_logFilenameFormat; }
};

class LocalLogRepository : public LogRepositoryBase {
  public:
    explicit LocalLogRepository(LocalFSLogFilePathProvider pathProvider, std::string password = "");

    [[nodiscard]] std::optional<LogFile>
    read(const std::chrono::year_month_day &date) const override;
    void remove(const std::chrono::year_month_day &date) override;
    void write(const LogFile &log) override;

  private:
    LocalFSLogFilePathProvider m_pathProvider;
    std::string m_password;
};

} // namespace caps_log::log
