#include "local_log_repository.hpp"

#include "log/log_repository_crypto_applier.hpp"
#include <fstream>
#include <sstream>
#include <utility>
#include <utils/crypto.hpp>

namespace caps_log::log {

namespace {
std::string readFileContent(const std::filesystem::path &path) {
    std::ifstream ifs{path};
    if (not ifs.is_open()) {
        throw std::runtime_error{"Failed to open file: " + path.string()};
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

std::chrono::year_month_day lastWriteTime(const std::filesystem::path &filePath) {
    // Check if the file exists
    if (not std::filesystem::exists(filePath)) {
        throw std::runtime_error{"File does not exist: " + filePath.string()};
    }

    // Get the last write time
    auto ftime = std::filesystem::last_write_time(filePath);

    // Convert to system time
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

    // Convert to a `year_month_day` format
    auto ymd = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(sctp)};
    return ymd;
}
} // namespace

std::filesystem::path validatedScratchpadPath(const std::filesystem::path &scratchpadDirectory,
                                              std::string_view name) {
    const bool hasDirectorySeparator =
        name.find('/') != std::string_view::npos || name.find('\\') != std::string_view::npos;
    if (name.empty() || name == "." || name == ".." || hasDirectorySeparator ||
        name.find('\0') != std::string_view::npos) {
        throw std::invalid_argument{
            "Invalid scratchpad name: expected a filename without directory components"};
    }

    const std::filesystem::path namePath{std::string{name}};
    if (namePath.has_root_path() || namePath.has_parent_path() || namePath.filename() != namePath) {
        throw std::invalid_argument{
            "Invalid scratchpad name: expected a filename without directory components"};
    }

    return scratchpadDirectory / namePath;
}

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
    if (not std::filesystem::exists(path.parent_path())) {
        std::error_code error;
        if (not std::filesystem::create_directories(path.parent_path(), error)) {
            throw std::runtime_error{"Failed to create directories for log file: " +
                                     error.message()};
        }
    }

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

Scratchpads LocalScratchpadRepository::read() const {
    Scratchpads scratchpads;
    for (const auto &entry : std::filesystem::directory_iterator(m_scratchpadDirPath)) {
        if (entry.is_regular_file()) {
            Scratchpad scratchpad;
            scratchpad.title = entry.path().filename();
            if (m_password.empty()) {
                scratchpad.content = readFileContent(entry.path());
            } else {
                std::ifstream ifs{entry.path()};
                if (not ifs.is_open()) {
                    throw std::runtime_error{"Failed to open scratchpad file: " +
                                             entry.path().string()};
                }
                scratchpad.content = utils::decrypt(m_password, ifs);
            }
            scratchpad.dateModified = lastWriteTime(entry.path());
            scratchpads.push_back(scratchpad);
        }
    }
    return std::move(scratchpads);
}

LocalScratchpadRepository::LocalScratchpadRepository(std::filesystem::path scratchpadDirPath,
                                                     std::string password)
    : m_scratchpadDirPath{std::move(scratchpadDirPath)}, m_password{std::move(password)} {
    if (!std::filesystem::exists(m_scratchpadDirPath)) {
        std::filesystem::create_directories(m_scratchpadDirPath);
    }
}

void LocalScratchpadRepository::remove(std::string name) {
    const auto path = validatedScratchpadPath(m_scratchpadDirPath, name);
    if (std::filesystem::exists(path)) {
        std::error_code error;
        std::filesystem::remove(path, error);
        if (error) {
            throw std::runtime_error{"Failed to remove scratchpad: " + error.message()};
        }
    } else {
        throw std::runtime_error{"Scratchpad does not exist: " + path.string()};
    }
}

void LocalScratchpadRepository::rename(std::string oldName, std::string newName) {
    const auto path = validatedScratchpadPath(m_scratchpadDirPath, oldName);
    const auto newPath = validatedScratchpadPath(m_scratchpadDirPath, newName);
    if (std::filesystem::exists(path)) {
        std::error_code error;
        std::filesystem::rename(path, newPath, error);
        if (error) {
            throw std::runtime_error{"Failed to rename scratchpad: " + error.message()};
        }
    } else {
        throw std::runtime_error{"Scratchpad does not exist: " + path.string()};
    }
}

} // namespace caps_log::log
