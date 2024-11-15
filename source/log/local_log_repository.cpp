#include "local_log_repository.hpp"

#include "log/log_repository_crypto_applier.hpp"
#include <fstream>
#include <sstream>
#include <utility>
#include <utils/crypto.hpp>

namespace caps_log::log {

LocalLogRepository::LocalLogRepository(LocalFSLogFilePathProvider pathProvider,
                                       std::string password)
    : m_pathProvider(std::move(pathProvider)), m_password{std::move(password)} {
    std::filesystem::create_directories(m_pathProvider.getLogDirPath());
    const auto isEncrypted =
        LogRepositoryCryptoApplier::isEncrypted(m_pathProvider.getLogDirPath());
    if (m_password.empty()) {
        if (isEncrypted) {
            throw std::runtime_error{"Password is required to open encrypted log repository!"};
        }
    } else {
        if (not isEncrypted) {
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
    const auto path = m_pathProvider.path(log.getDate());
    std::filesystem::create_directories(path.parent_path());

    if (not m_password.empty()) {
        std::istringstream iss{log.getContent()};
        std::ofstream{path} << utils::encrypt(m_password, iss);
    } else {
        std::ofstream{path} << log.getContent();
    }
}

void LocalLogRepository::remove(const std::chrono::year_month_day &date) {
    std::ignore = std::remove(m_pathProvider.path(date).c_str());
}

} // namespace caps_log::log
