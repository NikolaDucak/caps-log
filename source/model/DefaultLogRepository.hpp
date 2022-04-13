#pragma once

#include "LogRepositoryBase.hpp"

#include <filesystem>
#include <map>

namespace clog::model {

class DefaultLogRepository : public LogRepositoryBase {
public:
    static const std::string DEFAULT_LOG_DIR_PATH;
    static const std::string DEFAULT_LOG_FILENAME_FORMAT;

    DefaultLogRepository();

    YearLogEntryData collectDataForYear(unsigned year) override;
    void injectDataForDate(YearLogEntryData& data, const Date& date) override;
    std::optional<LogFile> readLogFile(const Date& date) override;
    LogFile readOrMakeLogFile(const Date& date) override;
    void removeLog(const Date& date) override;
    std::string path(const Date& date) override;

private:
    std::string readFile(const Date& date);

    std::filesystem::path dateToLogFilePath(const Date& d) const;
};

}  // namespace clog::model
