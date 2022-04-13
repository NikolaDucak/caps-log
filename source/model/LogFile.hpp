#pragma once

#include "Date.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace clog::model {

class LogFile {
    friend class LogRepository;
    std::string m_content;
public:
    static const std::string LOG_FILE_TITLE_DATE_FORMAT;
    static const std::string LOG_FILE_BASE_TEMPLATE;

    bool hasMeaningfullContent();
    LogFile(std::string content) :
        m_content(content) {}

    LogFile(const Date& date, std::string content) :
        m_content(std::move(content)) {}

    std::string getContent() { return m_content; }

private:
};

}  // namespace clog::model
