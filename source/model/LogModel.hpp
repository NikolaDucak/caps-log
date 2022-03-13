#pragma once

#include "LogRepository.hpp"

#include <map>
#include <optional>
#include <memory>

namespace clog::model {


class LogModel {
public:
    std::shared_ptr<LogRepository> m_filesystem;
    LogModel(std::shared_ptr<LogRepository> fs);

    YearLogEntryData getDataforYear(unsigned year);
    void injectDataForDate(YearLogEntryData& data, const Date& date);
    std::string getFilePathForLogEntry(const Date& date);
};

}  // namespace clog::model
