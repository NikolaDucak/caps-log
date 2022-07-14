#pragma once

#include "log_repository_base.hpp"

#include <filesystem>
#include <map>

namespace clog::model {

class DefaultLogRepository : public LogRepositoryBase {
  public:
    static const std::string DEFAULT_LOG_DIR_PATH;
    static const std::string DEFAULT_LOG_FILENAME_FORMAT;

    /**
     * @param logDirectory Directory in which individiaul log entries are stored.
     * @param logFilenameFormat Format used to generate names for log entries,
     * that format is used with Date::formatToString that.
     */
    DefaultLogRepository(std::string logDirectory = DEFAULT_LOG_DIR_PATH,
                         std::string logFilenameFormat = DEFAULT_LOG_FILENAME_FORMAT);

    YearOverviewData collectYearOverviewData(unsigned year) const override;
    void injectOverviewDataForDate(YearOverviewData &data, const Date &date) const override;
    std::optional<LogFile> read(const Date &date) const override;
    void remove(const Date &date) override;
    void write(const LogFile& log) override {}
    std::string path(const Date &date) override;

  private:
    std::filesystem::path dateToLogFilePath(const Date &d) const;

    std::string m_logDirectory;
    std::string m_logFilenameFormat;
};

} // namespace clog::model
