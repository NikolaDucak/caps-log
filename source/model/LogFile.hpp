#pragma once

#include "Date.hpp"
#include "LogRepository.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace clog::model {

struct Task {
    std::string title;
    std::string time_info;
    char completion_value = 0;
};

inline bool operator==(const Task& l, const Task& r) {
    return l.title == r.title && l.time_info == r.time_info &&
           r.completion_value == l.completion_value;
}

class LogEntryFile {
    Date m_date;
    std::shared_ptr<LogRepository> m_repo;

public:
    static const std::string LOG_FILE_TITLE_DATE_FORMAT;
    static const std::string LOG_FILE_BASE_TEMPLATE;

    LogEntryFile(std::shared_ptr<LogRepository> repo, const Date& date) :
        m_date(date), m_repo(std::move(repo)) {}

    bool hasData();
    void removeFile();

    //std::string get_path() { return m_repo->getFilePath(m_date); }

    std::vector<std::string> get_non_empty_section_titles();
    std::vector<Task> get_tasks();

};

}  // namespace clog::model
