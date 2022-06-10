#include "default_log_repository.hpp"

#include <fstream>
#include <iostream>
#include <regex>

namespace clog::model {

namespace {
std::string trim(const std::string& str, const std::string& whitespace = " \t\n") {
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return "";  // no content

    const auto strEnd   = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

std::string lowercase(std::string data) {
    std::transform(data.begin(), data.end(), data.begin(),
    [](unsigned char c){ return std::tolower(c); });
    return data;
}

auto sanitize(std::string& s) { return trim(s); }

}  // namespace

const std::string DefaultLogRepository::DEFAULT_LOG_DIR_PATH =
    std::getenv("HOME") + std::string { "/.clog/day" };

const std::string DefaultLogRepository::DEFAULT_LOG_FILENAME_FORMAT = "d%d_%m_%Y.md";

const static auto SECTION_TITLE_REGEX = std::regex { "^# \\s*(.*?)\\s*$" };
const static auto TASK_REGEX          = std::regex {
    R"(^ *(- )?\[(.)] *(\(([[a-zA-Z0-9_:]*)\))? *([\sa-zA-Z0-9_-]*)( *):?( *)(.*))"
};  // task title group 3
const static auto TASK_TITLE_MATCH { 5 };
const static auto SECTION_TITLE_MATCH { 1 };

DefaultLogRepository::DefaultLogRepository( std::string logDirectory, std::string logFilenameFormat) 
    : m_logDirectory(logDirectory), m_logFilenameFormat(logFilenameFormat) {
    std::filesystem::create_directories(m_logDirectory);
}

std::filesystem::path DefaultLogRepository::dateToLogFilePath(const Date& d) const {
    return { m_logDirectory + "/" + d.formatToString(m_logFilenameFormat) };
}

void DefaultLogRepository::injectDataForDate(YearLogEntryData& data, const Date& date) {
    std::ifstream input(dateToLogFilePath(date));

    // see if the log is available for that date and update the map
    if (not input.is_open()) {
        data.logAvailabilityMap.set(date, false);
        return;
    } else {
        data.logAvailabilityMap.set(date, true);
    }

    // clear all tasks and sections for that date
    for (auto task = data.taskMap.begin(); task != data.taskMap.end();) {
        task->second.set(date, false);
        if (not task->second.hasAnyDaySet()) {
            task = data.taskMap.erase(task);
        } else {
            task++;
        }
    }
    for (auto section = data.sectionMap.begin(); section != data.sectionMap.end();) {
        section->second.set(date, false);
        if (not section->second.hasAnyDaySet()) {
            section = data.sectionMap.erase(section);
        } else {
            section++;
        }
    }

    // now read update the task and section map
    std::string line;
    getline(input, line);  // skip first line, because I like it that way

    while (getline(input, line)) {
        line = sanitize(line);
        std::smatch sm;

        if (std::regex_match(line, sm, TASK_REGEX)) {
            data.taskMap[lowercase(sm[TASK_TITLE_MATCH])].set(date, true);
        }

        if (std::regex_match(line, sm, SECTION_TITLE_REGEX)) {
            data.sectionMap[lowercase(sm[SECTION_TITLE_MATCH])].set(date, true);
        }
    }
}

YearLogEntryData DefaultLogRepository::collectDataForYear(unsigned year) {
    YearLogEntryData d;
    for (unsigned month = Month::JANUARY; month <= Month::DECEMBER; month++) {
        for (unsigned day = 1; day <= getNumberOfDaysForMonth(month, year); day++) {
            injectDataForDate(d, Date { day, month, year });
        }
    }
    return std::move(d);
}

std::optional<LogFile> DefaultLogRepository::readLogFile(const Date& date) {
    std::ifstream t(dateToLogFilePath(date));
    if (t.is_open()) {
        std::stringstream buffer;
        buffer << t.rdbuf();
        return LogFile{date, std::string{buffer.str()}};
    } else {
        return {};
    }
}

LogFile DefaultLogRepository::readOrMakeLogFile(const Date& date) {
    std::ifstream t(dateToLogFilePath(date));
    if (t.is_open()) {
        std::stringstream buffer;
        buffer << t.rdbuf();
        return LogFile{buffer.str()};
    } else {
        return LogFile{""};
    }
}

std::string DefaultLogRepository::readFile(const Date& date) {
    std::ifstream t(dateToLogFilePath(date));
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

void DefaultLogRepository::removeLog(const Date& date) {
    auto result = std::remove(dateToLogFilePath(date).c_str());
}

std::string DefaultLogRepository::path(const Date& date) {
    return dateToLogFilePath(date).string();
}

}  // namespace clog::model
