#include "utils/git_repo.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <git2.h>
#include <gtest/gtest.h>

namespace caps_log::test::e2e {
namespace {

using RepositoryPtr = std::unique_ptr<git_repository, decltype(&git_repository_free)>;
using ConfigPtr = std::unique_ptr<git_config, decltype(&git_config_free)>;
using RemotePtr = std::unique_ptr<git_remote, decltype(&git_remote_free)>;

void checkGitError(int code) {
    if (code >= 0) {
        return;
    }

    const git_error *error = git_error_last();
    throw std::runtime_error{error != nullptr ? error->message : "Unknown libgit2 error"};
}

RepositoryPtr openRepository(const std::filesystem::path &path) {
    git_repository *repo = nullptr;
    checkGitError(git_repository_open(&repo, path.c_str()));
    return RepositoryPtr{repo, &git_repository_free};
}

void writeFile(const std::filesystem::path &path, const std::string &content) {
    std::ofstream file{path};
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    file << content;
}

std::vector<std::string> getConfigValues(const std::filesystem::path &repoPath, const char *name) {
    auto repo = openRepository(repoPath);

    git_config *rawConfig = nullptr;
    checkGitError(git_repository_config(&rawConfig, repo.get()));
    ConfigPtr config{rawConfig, &git_config_free};

    std::vector<std::string> values;
    const auto collectValue = [](const git_config_entry *entry, void *payload) {
        auto &output = *static_cast<std::vector<std::string> *>(payload);
        output.emplace_back(entry->value);
        return 0;
    };

    const int result =
        git_config_get_multivar_foreach(config.get(), name, nullptr, collectValue, &values);
    if (result != GIT_ENOTFOUND) {
        checkGitError(result);
    }
    return values;
}

git_oid referenceId(const std::filesystem::path &repoPath, const char *referenceName) {
    auto repo = openRepository(repoPath);
    git_oid oid{};
    checkGitError(git_reference_name_to_id(&oid, repo.get(), referenceName));
    return oid;
}

class GitRepoE2ETest : public testing::Test {
  protected:
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void SetUpTestSuite() { checkGitError(git_libgit2_init()); }

    // NOLINTNEXTLINE(readability-identifier-naming)
    static void TearDownTestSuite() { git_libgit2_shutdown(); }

    // NOLINTNEXTLINE(readability-identifier-naming,readability-convert-member-functions-to-static)
    void SetUp() override {
        constexpr unsigned kIsBare = 0U;
        constexpr unsigned kIsNotBare = 1U;
        static std::atomic_uint testNumber{};
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        m_testRoot = std::filesystem::temp_directory_path() /
                     ("caps_log_git_repo_e2e_" + std::to_string(timestamp) + "_" +
                      std::to_string(testNumber.fetch_add(1)));
        m_workingRepoPath = m_testRoot / "working";
        m_remoteRepoPath = m_testRoot / "remote.git";
        m_privateKeyPath = m_testRoot / "id_test";
        m_publicKeyPath = m_testRoot / "id_test.pub";

        std::filesystem::create_directories(m_testRoot);
        writeFile(m_privateKeyPath, "unused test key");
        writeFile(m_publicKeyPath, "unused test public key");

        git_repository *rawRemoteRepo = nullptr;
        checkGitError(git_repository_init(&rawRemoteRepo, m_remoteRepoPath.c_str(), kIsBare));
        RepositoryPtr remoteRepo{rawRemoteRepo, &git_repository_free};

        git_repository *rawWorkingRepo = nullptr;
        checkGitError(git_repository_init(&rawWorkingRepo, m_workingRepoPath.c_str(), kIsNotBare));
        RepositoryPtr workingRepo{rawWorkingRepo, &git_repository_free};
        checkGitError(git_repository_set_head(workingRepo.get(), "refs/heads/main"));

        git_config *rawConfig = nullptr;
        checkGitError(git_repository_config(&rawConfig, workingRepo.get()));
        ConfigPtr config{rawConfig, &git_config_free};
        checkGitError(git_config_set_string(config.get(), "user.name", "Caps Log Test"));
        checkGitError(git_config_set_string(config.get(), "user.email", "caps-log@example.com"));

        git_remote *rawRemote = nullptr;
        checkGitError(
            git_remote_create(&rawRemote, workingRepo.get(), "origin", m_remoteRepoPath.c_str()));
        RemotePtr remote{rawRemote, &git_remote_free};
    }

    // NOLINTNEXTLINE(readability-identifier-naming,readability-convert-member-functions-to-static)
    void TearDown() override { std::filesystem::remove_all(m_testRoot); }

    [[nodiscard]] utils::GitRepoConfig gitRepoConfig() const {
        return {
            .root = m_workingRepoPath,
            .sshKeyPath = m_privateKeyPath,
            .sshPubKeyPath = m_publicKeyPath,
            .mainBranchName = "main",
            .remoteName = "origin",
        };
    }

    // Test fixtures expose state to the test class generated by GoogleTest.
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes,readability-identifier-naming)
    std::filesystem::path m_testRoot;
    std::filesystem::path m_workingRepoPath;
    std::filesystem::path m_remoteRepoPath;
    std::filesystem::path m_privateKeyPath;
    std::filesystem::path m_publicKeyPath;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes,readability-identifier-naming)
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,readability-function-cognitive-complexity)
TEST_F(GitRepoE2ETest, RepeatedPushesReachRemoteWithoutAddingPushRefspecs) {
    writeFile(m_workingRepoPath / "log.md", "first revision\n");

    {
        utils::GitRepo repo{gitRepoConfig()};
        ASSERT_TRUE(repo.commitAll());
        repo.push();

        writeFile(m_workingRepoPath / "log.md", "second revision\n");
        ASSERT_TRUE(repo.commitAll());
        repo.push();
        EXPECT_FALSE(repo.commitAll());
    }

    EXPECT_TRUE(getConfigValues(m_workingRepoPath, "remote.origin.push").empty());

    const auto localHead = referenceId(m_workingRepoPath, "refs/heads/main");
    const auto remoteHead = referenceId(m_remoteRepoPath, "refs/heads/main");
    EXPECT_EQ(git_oid_cmp(&localHead, &remoteHead), 0);
}

} // namespace
} // namespace caps_log::test::e2e
