#include "local_log_repository.hpp"

#include "utils/string.hpp"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace clog::model {

using namespace date;

const std::string LocalFSLogFilePathProvider::DEFAULT_LOG_DIR_PATH =
    std::getenv("HOME") + std::string{"/.clog/day"};

const std::string LocalFSLogFilePathProvider::DEFAULT_LOG_FILENAME_FORMAT = "d%Y_%m_%d.md";

LocalLogRepository::LocalLogRepository(LocalFSLogFilePathProvider pathProvider)
    : m_pathProvider(std::move(pathProvider)) {
    std::filesystem::create_directories(m_pathProvider.getLogDirPath());
}

std::optional<LogFile> LocalLogRepository::read(const Date &date) const {
    if (std::ifstream t{m_pathProvider.path(date)}; t.is_open()) {
        std::stringstream buffer;
        buffer << t.rdbuf();
        return LogFile{date, std::string{buffer.str()}};
    }
    return {};
}

void LocalLogRepository::write(const LogFile &log) {
    std::ofstream{m_pathProvider.path(log.getDate())} << log.getContent();
}

void LocalLogRepository::remove(const Date &date) {
    (void)std::remove(m_pathProvider.path(date).c_str());
}

} // namespace clog::model
