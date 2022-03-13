#pragma once

#include "LogRepository.hpp"

#include <filesystem>
#include <map>

namespace clog::model {

class DefaultLogRepository : public LogRepository {
public:
    static const std::string DEFAULT_LOG_DIR_PATH;
    static const std::string DEFAULT_LOG_FILENAME_FORMAT;

    DefaultLogRepository();
    ~DefaultLogRepository() override {};

    YearLogEntryData collectDataForYear(unsigned year) override;
    void injectDataForDate(YearLogEntryData& data, const Date& date) override;
    std::string getLogEntryPath(const Date& date) override;


private:

    std::string readFile(const Date& date) override;
    void removeFile(const Date& date) override;

    std::filesystem::path dateToLogFilePath(const Date& d) const;
};

}  // namespace clog::model
