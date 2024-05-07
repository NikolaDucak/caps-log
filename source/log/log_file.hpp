#pragma once

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

namespace caps_log::log {

/*
 * Represents a log file with a specific date and content.
 * The content is a string that contains the entire log file (in markdown format) and
 * the date is the date of the log file.
 */
class LogFile {
    std::string m_content;
    std::chrono::year_month_day m_date;

  public:
    LogFile(const std::chrono::year_month_day &date, std::string content)
        : m_date{date}, m_content{std::move(content)} {}

    std::string getContent() const { return m_content; }
    std::chrono::year_month_day getDate() const { return m_date; }

    std::vector<std::string> readTagTitles() const;

    std::vector<std::string> readSectionTitles(bool skipFirstLine = true) const;
};

} // namespace caps_log::log
