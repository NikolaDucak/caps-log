#pragma once

#include <chrono>
#include <map>
#include <set>
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
    std::map<std::string, std::set<std::string>> m_tagsPerSection;
    std::chrono::year_month_day m_date;

  public:
    LogFile(const std::chrono::year_month_day &date, std::string content)
        : m_date{date}, m_content{std::move(content)} {}

    LogFile &parse(bool skipFirstLine = true);

    std::string getContent() const { return m_content; }
    std::chrono::year_month_day getDate() const { return m_date; }

    static constexpr std::string_view kRootSectionKey = "<root section>";
    std::set<std::string> getSectionTitles() const;
    std::set<std::string> getTagTitles() const;
    std::map<std::string, std::set<std::string>> getTagsPerSection() const;
};

} // namespace caps_log::log
