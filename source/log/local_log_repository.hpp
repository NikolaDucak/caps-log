#pragma once

#include "log_repository_base.hpp"

#include <filesystem>
#include <map>
#include <optional>

namespace caps_log::model {

/**
 * Helps to have a one place for getting paths for both LocalLogRepository and any other editor that
 * wants to log. Previously repo base had path member and it kinda stuck out. not all repos have a
 * path like filesystem
 */
class LocalFSLogFilePathProvider {
    std::string m_logDirectory, m_logFilenameFormat;

  public:
    LocalFSLogFilePathProvider(std::string logDir, std::string logFilenameFormat)
        : m_logDirectory{std::move(logDir)}, m_logFilenameFormat{std::move(logFilenameFormat)} {}

    inline std::string path(const date::Date &date) const {
        return {m_logDirectory + "/" + date.formatToString(m_logFilenameFormat)};
    }

    inline std::string getLogDirPath() const { return m_logDirectory; }
    inline std::string getLogFilenameFormat() const { return m_logFilenameFormat; }
};

class LocalLogRepository : public LogRepositoryBase {
  public:
    static const std::string DEFAULT_LOG_DIR_PATH;
    static const std::string DEFAULT_LOG_FILENAME_FORMAT;

    /**
     * @param logDirectory Directory in which individiaul log entries are stored.
     * @param logFilenameFormat Format used to generate names for log entries,
     * that format is used with Date::formatToString that.
     */
    LocalLogRepository(LocalFSLogFilePathProvider pathProvider, std::string password = "");

    std::optional<LogFile> read(const date::Date &date) const override;
    void remove(const date::Date &date) override;
    void write(const LogFile &log) override;

  private:
    LocalFSLogFilePathProvider m_pathProvider;
    std::string m_password;
};

} // namespace caps_log::model
