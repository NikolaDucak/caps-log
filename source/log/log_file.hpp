#pragma once

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

namespace caps_log::log {

class LogFile {
    std::string m_content;
    std::chrono::year_month_day m_date;

  public:
    LogFile(const std::chrono::year_month_day &date, std::string content)
        : m_date{date}, m_content{std::move(content)} {}

    std::string getContent() const { return m_content; }
    std::chrono::year_month_day getDate() const { return m_date; }

    std::vector<std::string> readTagTitles() const {
        std::stringstream sstream{m_content};
        return readTagTitles(sstream);
    }

    std::vector<std::string> readSectionTitles(bool skipFirstLine = true) const {
        std::stringstream sstream{m_content};
        return readSectionTitles(sstream, skipFirstLine);
    }

    bool hasMeaningfullContent();

    // TODO: cleanup
    static std::vector<std::string> readTagTitles(std::istream &input);
    static std::vector<std::string> readSectionTitles(std::istream &input,
                                                      bool skipFirstLine = true);

    static std::vector<std::string> readTagTitles(const std::string &input) {
        std::stringstream sstream{input};
        return readTagTitles(sstream);
    }

    static std::vector<std::string> readSectionTitles(const std::string &input,
                                                      bool skipFirstLine = true) {
        std::stringstream sstream{input};
        return readSectionTitles(sstream, skipFirstLine);
    }
};

} // namespace caps_log::log
