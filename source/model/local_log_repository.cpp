#include "local_log_repository.hpp"

#include "utils/string.hpp"
#include <fstream>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <regex>
#include <sstream>
#include <utility>
#include <utils/crypto.hpp>

namespace clog::model {

using namespace date;

LocalLogRepository::LocalLogRepository(LocalFSLogFilePathProvider pathProvider,
                                       std::string password)
    : m_pathProvider(std::move(pathProvider)), m_password{std::move(password)} {
    std::filesystem::create_directories(m_pathProvider.getLogDirPath());
}

std::optional<LogFile> LocalLogRepository::read(const Date &date) const {
    std::ifstream t{m_pathProvider.path(date)};

    if (not t.is_open()) {
        return std::nullopt;
    }

    if (not m_password.empty()) {
        return LogFile{date, utils::decrypt(m_password, t)};
    }

    std::stringstream buffer;
    buffer << t.rdbuf();
    return LogFile{date, std::string{buffer.str()}};
}

void LocalLogRepository::write(const LogFile &log) {
    if (not m_password.empty()) {
        std::istringstream iss{log.getContent()};
        std::ofstream{m_pathProvider.path(log.getDate())} << utils::encrypt(m_password, iss);
    } else {
        std::ofstream{m_pathProvider.path(log.getDate())} << log.getContent();
    }
}

void LocalLogRepository::remove(const Date &date) {
    std::ignore = std::remove(m_pathProvider.path(date).c_str());
}

} // namespace clog::model
