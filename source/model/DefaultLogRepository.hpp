#pragma once

#include "LogRepositoryBase.hpp"

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

    YearLogEntryData collectDataForYear(unsigned year) override;
    void injectDataForDate(YearLogEntryData& data, const Date& date) override;
    std::optional<LogFile> readLogFile(const Date& date) override;
    LogFile readOrMakeLogFile(const Date& date) override;
    void removeLog(const Date& date) override;
    std::string path(const Date& date) override;

private:
    std::string readFile(const Date& date);
    std::filesystem::path dateToLogFilePath(const Date& d) const;

    std::string m_logDirectory;
    std::string m_logFilenameFormat;
};

}  // namespace clog::model
