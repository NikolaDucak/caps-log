#include "local_log_repository.hpp"

#include "log/log_repository_crypto_applyer.hpp"
#include "utils/string.hpp"
#include <fstream>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <regex>
#include <sstream>
#include <utility>
#include <utils/crypto.hpp>

namespace caps_log::model {

using namespace date;

LocalLogRepository::LocalLogRepository(LocalFSLogFilePathProvider pathProvider,
                                       std::string password)
    : m_pathProvider(std::move(pathProvider)), m_password{std::move(password)} {
    // create log directory if it isn't already created
    std::filesystem::create_directories(m_pathProvider.getLogDirPath());
    // in case that there is an encryption marker file & no password is provided, throw
    auto clePath =
        m_pathProvider.getLogDirPath() / LogRepositoryCryptoApplier::encryptetLogRepoMarkerFile;
    if (std::filesystem::exists(clePath) && m_password.empty()) {
        throw std::runtime_error{"Password is required to open encrypted log repository!"};
    }
    // in case that the decrypted contents of the encryptetLogRepoMarkerFile does not start
    // with encryption marker, throw due to invalid password
    if (std::filesystem::exists(clePath) && not m_password.empty()) {
        auto cleStream = std::ifstream{clePath};
        auto decryptedMarker = utils::decrypt(m_password, cleStream);
        if (decryptedMarker.find(LogRepositoryCryptoApplier::encryptetLogRepoMarker) != 0) {
            throw std::runtime_error{"Invalid password provided!"};
        }
    }
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

} // namespace caps_log::model
