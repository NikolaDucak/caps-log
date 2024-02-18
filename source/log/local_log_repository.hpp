#pragma once

#include "log_repository_base.hpp"
#include "utils/date.hpp"

#include <filesystem>
#include <optional>

namespace caps_log::log {

/**
 * Helps to have a one place for getting paths for both LocalLogRepository and any other editor that
 * wants to log. Previously repo base had path member and it kinda stuck out. not all repos have a
 * path like filesystem
 */
class LocalFSLogFilePathProvider {
    std::filesystem::path m_logDirectory;
    std::string m_logFilenameFormat;

  public:
    LocalFSLogFilePathProvider(const std::filesystem::path &logDir, std::string logFilenameFormat)
        : m_logDirectory{logDir}, m_logFilenameFormat{std::move(logFilenameFormat)} {}

    inline std::filesystem::path path(const std::chrono::year_month_day &date) const {
        return {m_logDirectory / utils::date::formatToString(date, m_logFilenameFormat)};
    }

    inline std::filesystem::path getLogDirPath() const { return m_logDirectory; }
    inline std::string getLogFilenameFormat() const { return m_logFilenameFormat; }
};

class LocalLogRepository : public LogRepositoryBase {
  public:
    static const std::string kDefaultLogDirPath;
    static const std::string kDefaultLogFilenameFormat;

    /**
     * @param logDirectory Directory in which individiaul log entries are stored.
     * @param logFilenameFormat Format used to generate names for log entries,
     * that format is used with Date::formatToString that.
     */
    LocalLogRepository(LocalFSLogFilePathProvider pathProvider, std::string password = "");

    std::optional<LogFile> read(const std::chrono::year_month_day &date) const override;
    void remove(const std::chrono::year_month_day &date) override;
    void write(const LogFile &log) override;

  private:
    LocalFSLogFilePathProvider m_pathProvider;
    std::string m_password;
};

} // namespace caps_log::log
