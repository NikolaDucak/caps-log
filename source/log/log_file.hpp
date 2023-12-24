#pragma once

#include "date/date.hpp"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace caps_log::model {

class LogFile {
    std::string m_content;
    caps_log::date::Date m_date;

  public:
    LogFile(const caps_log::date::Date &date, std::string content)
        : m_date{date}, m_content{std::move(content)} {}

    std::string getContent() const { return m_content; }
    caps_log::date::Date getDate() const { return m_date; }

    std::vector<std::string> readTagTitles() {
        std::stringstream ss{m_content};
        return readTagTitles(ss);
    }

    std::vector<std::string> readSectionTitles(bool skipFirstLine = true) {
        std::stringstream ss{m_content};
        return readSectionTitles(ss, skipFirstLine);
    }

    bool hasMeaningfullContent();

    // TODO: cleanup
    static std::vector<std::string> readTagTitles(std::istream &input);
    static std::vector<std::string> readSectionTitles(std::istream &input,
                                                      bool skipFirstLine = true);

    static std::vector<std::string> readTagTitles(const std::string &input) {
        std::stringstream ss{input};
        return readTagTitles(ss);
    }

    static std::vector<std::string> readSectionTitles(const std::string &input,
                                                      bool skipFirstLine = true) {
        std::stringstream ss{input};
        return readSectionTitles(ss, skipFirstLine);
    }
};

} // namespace caps_log::model
