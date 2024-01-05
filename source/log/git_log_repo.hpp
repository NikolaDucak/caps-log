#pragma once

#include "local_log_repository.hpp"
#include "log_repository_base.hpp"

#include <filesystem>
#include <git2.h>
#include <optional>

namespace caps_log::log {

struct GitLogRepositoryConfig {
    std::filesystem::path sshKeyPath;
    std::filesystem::path sshPubKeyPath;
    std::string remoteName;
    std::string mainBranchName;
};

/**
 * A utility class that manages the initialization of libgit2 and other required interactions with
 * the library. It deals with commit the files, pulling and pushing. It expect that the repository
 * has an innitial commit, remote added and ssh communication.
 */
class GitRepo {
    git_repository *m_repo = nullptr;
    std::string m_remoteName;
    std::string m_mainBranchName;
    std::string m_sshPubKeyPath;
    std::string m_sshKeyPath;

  public:
    GitRepo(const std::filesystem::path &path, std::string sshKeyPath, std::string sshPubKeyPath,
            std::string remoteName = "origin", std::string mainBranchName = "master");

    GitRepo(const GitRepo &) = delete;
    GitRepo(GitRepo &&) = default;
    GitRepo &operator=(const GitRepo &) = delete;
    GitRepo &operator=(GitRepo &&) = default;
    ~GitRepo();
    void push();
    bool commitAll();
    void pull();

  private:
    git_remote_callbacks getRemoteCallbacks();
    int credentialsCallback(git_cred **cred, const char *url, const char *username_from_url,
                            unsigned int allowed_types);
    static int acquireCredentials(git_cred **cred, const char *url, const char *username_from_url,
                                  unsigned int allowed_types, void *payload);
};

class GitLogRepository final : public LogRepositoryBase {
    GitRepo repo;
    LocalLogRepository logRepo;
    GitLogRepositoryConfig config;

  public:
    GitLogRepository(std::string root, const LocalFSLogFilePathProvider &pathProvider,
                     std::string pass, const GitLogRepositoryConfig &conf);

    GitLogRepository(const GitLogRepository &) = delete;
    GitLogRepository(GitLogRepository &&) = default;
    GitLogRepository &operator=(const GitLogRepository &) = delete;
    GitLogRepository &operator=(GitLogRepository &&) = default;

    ~GitLogRepository() override;
    std::optional<LogFile> read(const date::Date &date) const override;
    void remove(const date::Date &date) override;
    void write(const LogFile &log) override;
};

} // namespace caps_log::log
