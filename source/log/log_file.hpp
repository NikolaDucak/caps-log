#pragma once

#include <chrono>
#include <map>
#include <set>
#include <string>

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

    [[nodiscard]] std::string getContent() const { return m_content; }
    [[nodiscard]] std::chrono::year_month_day getDate() const { return m_date; }

    [[nodiscard]] std::set<std::string> getSectionTitles() const;
    [[nodiscard]] std::set<std::string> getTagTitles() const;
    [[nodiscard]] std::map<std::string, std::set<std::string>> getTagsPerSection() const;

    static constexpr std::string_view kRootSectionKey = "<root section>";
};

} // namespace caps_log::log
