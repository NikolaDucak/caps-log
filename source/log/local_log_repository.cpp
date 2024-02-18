#include "local_log_repository.hpp"

#include "log/log_repository_crypto_applyer.hpp"
#include <fstream>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <utility>
#include <utils/crypto.hpp>

namespace caps_log::log {

LocalLogRepository::LocalLogRepository(LocalFSLogFilePathProvider pathProvider,
                                       std::string password)
    : m_pathProvider(std::move(pathProvider)), m_password{std::move(password)} {
    std::filesystem::create_directories(m_pathProvider.getLogDirPath());
    const auto kisEncrypted =
        LogRepositoryCryptoApplier::isEncrypted(m_pathProvider.getLogDirPath());
    if (m_password.empty()) {
        if (kisEncrypted) {
            throw std::runtime_error{"Password is required to open encrypted log repository!"};
        }
    } else {
        if (not kisEncrypted) {
            throw std::runtime_error{"Password provided for a non encrypted repository!"};
        }
        if (not LogRepositoryCryptoApplier::isDecryptionPasswordValid(
                m_pathProvider.getLogDirPath(), m_password)) {
            throw std::runtime_error{"Invalid password provided!"};
        }
    }
}

std::optional<LogFile> LocalLogRepository::read(const std::chrono::year_month_day &date) const {
    std::ifstream ifs{m_pathProvider.path(date)};

    if (not ifs.is_open()) {
        return std::nullopt;
    }

    if (not m_password.empty()) {
        return LogFile{date, utils::decrypt(m_password, ifs)};
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
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

void LocalLogRepository::remove(const std::chrono::year_month_day &date) {
    std::ignore = std::remove(m_pathProvider.path(date).c_str());
}

} // namespace caps_log::log
