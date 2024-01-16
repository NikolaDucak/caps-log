#pragma once

#include <filesystem>
#include <git2.h>
#include <optional>

namespace caps_log::utils {

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
    GitRepo(const std::filesystem::path &path, const std::filesystem::path &sshKeyPath,
            const std::filesystem::path &sshPubKeyPath, std::string remoteName = "origin",
            std::string mainBranchName = "master");

    GitRepo(const GitRepo &) = delete;
    GitRepo(GitRepo &&) noexcept;
    GitRepo &operator=(const GitRepo &) = delete;
    GitRepo &operator=(GitRepo &&) noexcept;
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

} // namespace caps_log::utils
