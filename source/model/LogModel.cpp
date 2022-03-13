#include "LogModel.hpp"

#include <functional>
#include <iostream>
#include <set>

namespace clog::model {

LogModel::LogModel(std::shared_ptr<LogRepository> fs) : m_filesystem(std::move(fs)) {}

YearLogEntryData LogModel::getDataforYear(unsigned year) {
    return m_filesystem->collectDataForYear(year);
}

void LogModel::injectDataForDate(YearLogEntryData& data, const Date& date) {
    m_filesystem->injectDataForDate(data, date);
}

std::string LogModel::getFilePathForLogEntry(const Date& date) {
    return m_filesystem->getLogEntryPath(date);
}

}  // namespace clog::model
