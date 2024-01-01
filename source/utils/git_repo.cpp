#include "git_repo.hpp"
#include <fmt/format.h>
#include <string>

#include <algorithm>
#include <iostream>
#include <utility>

namespace caps_log::utils {

void checkGitError(int code, int line, const char *file) {
    if (code < 0) {
        const git_error *error = git_error_last();
        throw std::runtime_error{std::string{"Git error @"} + file + ":" + std::to_string(line) +
                                 "\n\t" + error->message};
    }
}

// NOLINTNEXTLINE
#define CHECK_GIT_ERROR(error_code) ::caps_log::utils::checkGitError(error_code, __LINE__, __FILE__)

bool validateStr(const std::string &str) {
    return std::all_of(str.begin(), str.end(), [](auto chr) { return std::isprint(chr) != 0; });
}

git_signature *getSignatureFromRepoConfig(git_repository *repo) {
    // Load the repository configuration
    git_config *live_config = nullptr;
    CHECK_GIT_ERROR(git_repository_config(&live_config, repo));
    git_config *config = nullptr;
    git_config_snapshot(&config, live_config);

    // Retrieve user.name
    const char *config_name = nullptr;
    CHECK_GIT_ERROR(git_config_get_string(&config_name, config, "user.name") != 0);

    // Retrieve user.email
    const char *config_email = nullptr;
    CHECK_GIT_ERROR(git_config_get_string(&config_email, config, "user.email") != 0);

    if (not validateStr(config_name)) {
        throw std::runtime_error{std::string{"Invalid name from config: "} + config_name};
    }
    if (not validateStr(config_email)) {
        throw std::runtime_error{std::string{"Invalid email from config: "} + config_email};
    }

    // Create signature
    git_signature *sig = nullptr;
    CHECK_GIT_ERROR(git_signature_now(&sig, config_name, config_email));
    git_config_free(config);

    return sig;
}

bool hasChangedFiles(git_repository *repo) {
    git_status_list *status = NULL;
    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
                 GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    CHECK_GIT_ERROR(git_status_list_new(&status, repo, &opts));

    size_t status_count = git_status_list_entrycount(status);
    git_status_list_free(status);

    return status_count > 0;
}
GitRepo::GitRepo(const std::filesystem::path &path, std::string sshKeyPath,
                 std::string sshPubKeyPath, std::string remoteName, std::string mainBranchName)
    : m_remoteName{std::move(remoteName)}, m_mainBranchName{std::move(mainBranchName)},
      m_sshKeyPath{std::move(sshKeyPath)}, m_sshPubKeyPath{std::move(sshPubKeyPath)} {
    if (m_sshKeyPath.empty() || m_sshPubKeyPath.empty()) {
        throw std::invalid_argument{"SSH keys are not set!"};
    }
    if (m_remoteName.empty()) {
        throw std::invalid_argument{"Remote name can not be empty!"};
    }
    if (m_mainBranchName.empty()) {
        throw std::invalid_argument{"Main branch name can not be empty!"};
    }
    git_libgit2_init();
    CHECK_GIT_ERROR(git_repository_open(&m_repo, path.c_str()));
}

GitRepo::GitRepo(GitRepo &&other) noexcept
    : m_repo(other.m_repo), m_mainBranchName(std::move(other.m_mainBranchName)),
      m_remoteName{std::move(other.m_remoteName)}, m_sshKeyPath{std::move(other.m_sshKeyPath)},
      m_sshPubKeyPath{std::move(other.m_sshPubKeyPath)} {
    other.m_repo = nullptr;
}

GitRepo &GitRepo::operator=(GitRepo &&other) noexcept {
    this->m_repo = other.m_repo;
    this->m_sshPubKeyPath = std::move(other.m_sshPubKeyPath);
    this->m_sshKeyPath = std::move(other.m_sshKeyPath);
    this->m_remoteName = std::move(other.m_remoteName);
    this->m_mainBranchName = std::move(other.m_mainBranchName);

    other.m_repo = nullptr;

    return *this;
}

GitRepo::~GitRepo() {
    if (m_repo != nullptr) {
        git_repository_free(m_repo);
        // TODO: not every repo should do this...
        git_libgit2_shutdown();
    }
}

int GitRepo::credentialsCallback(git_cred **cred, const char *url, const char *username_from_url,
                                 unsigned int allowed_types) {
    if ((allowed_types & GIT_CREDTYPE_SSH_KEY) != 0U) {
        return git_cred_ssh_key_new(cred, username_from_url, this->m_sshPubKeyPath.c_str(),
                                    this->m_sshKeyPath.c_str(), nullptr);
    }
    return -1; // Unsupported credential type
}

int GitRepo::acquireCredentials(git_credential **cred, const char *url,
                                const char *username_from_url, unsigned int allowed_types,
                                void *payload) {
    return static_cast<GitRepo *>(payload)->credentialsCallback(cred, url, username_from_url,
                                                                allowed_types);
}

git_remote_callbacks GitRepo::getRemoteCallbacks() {
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    callbacks.credentials = &acquireCredentials;
    callbacks.payload = this;
    return callbacks;
}

void GitRepo::push() {
    git_remote *remote = nullptr;
    CHECK_GIT_ERROR(git_remote_lookup(&remote, m_repo, m_remoteName.c_str()));

    auto push_ref = "refs/heads/" + m_mainBranchName + ":refs/heads/" + m_mainBranchName;
    git_remote_add_push(m_repo, m_remoteName.c_str(), push_ref.c_str());

    git_push_options push_opts = GIT_PUSH_OPTIONS_INIT;
    const auto callbacks = getRemoteCallbacks();
    push_opts.callbacks = callbacks;

    auto refspec = std::string{"refs/heads/"} + m_mainBranchName;
    auto *tmp = refspec.data();
    const git_strarray refspecs = {&tmp, 1};

    CHECK_GIT_ERROR(git_remote_push(remote, &refspecs, &push_opts));
}

bool GitRepo::commitAll() {
    if (not hasChangedFiles(m_repo)) {
        return false;
    }

    auto *sig = getSignatureFromRepoConfig(m_repo);

    // Index related operations
    git_index *index = nullptr;
    CHECK_GIT_ERROR(git_repository_index(&index, m_repo));
    CHECK_GIT_ERROR(git_index_add_all(index, nullptr, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr));
    CHECK_GIT_ERROR(git_index_write(index));

    // Create tree
    git_oid tree_oid;
    git_oid commit_oid;
    git_tree *tree = nullptr;
    CHECK_GIT_ERROR(git_index_write_tree(&tree_oid, index));
    CHECK_GIT_ERROR(git_tree_lookup(&tree, m_repo, &tree_oid));

    // Commit
    git_oid parent_commit_oid;
    git_commit *parent_commit = nullptr;
    if (git_reference_name_to_id(&parent_commit_oid, m_repo, "HEAD") == 0) {
        CHECK_GIT_ERROR(git_commit_lookup(&parent_commit, m_repo, &parent_commit_oid));
    }

    static const char *commit_message = "caps-log auto push";
    // NOLINTNEXTLINE
    git_commit_create_v(&commit_oid, m_repo, "HEAD", sig, sig, nullptr, commit_message, tree,
                        parent_commit != nullptr ? 1 : 0, parent_commit);

    // Cleanup
    git_signature_free(sig);
    git_index_free(index);
    git_tree_free(tree);
    if (parent_commit != nullptr) {
        git_commit_free(parent_commit);
    }
    return true;
}

void GitRepo::pull() {
    git_remote *remote = nullptr;
    git_reference *local_ref = nullptr;
    git_reference *remote_ref = nullptr;
    git_annotated_commit *remote_annotated = nullptr;

    // Look up the remote
    CHECK_GIT_ERROR(git_remote_lookup(&remote, m_repo, m_remoteName.c_str()));

    // Fetch the changes from the remote
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    fetch_opts.callbacks = getRemoteCallbacks();
    CHECK_GIT_ERROR(git_remote_fetch(remote, nullptr, &fetch_opts, "Fetch"));

    // Get the reference for the remote tracking branch
    std::string remote_refname = "refs/remotes/" + m_remoteName + "/" + m_mainBranchName;
    CHECK_GIT_ERROR(git_reference_lookup(&remote_ref, m_repo, remote_refname.c_str()));

    // Create an annotated commit from the remote reference
    const git_oid *remote_oid = git_reference_target(remote_ref);
    CHECK_GIT_ERROR(git_annotated_commit_lookup(&remote_annotated, m_repo, remote_oid));

    // Get the reference for the local branch
    std::string local_refname = "refs/heads/" + m_mainBranchName;
    CHECK_GIT_ERROR(git_reference_lookup(&local_ref, m_repo, local_refname.c_str()));

    // Perform merge analysis
    git_merge_analysis_t analysis;                                  // NOLINT
    git_merge_preference_t preference;                              // NOLINT
    const git_annotated_commit *their_heads[] = {remote_annotated}; // NOLINT
    CHECK_GIT_ERROR(git_merge_analysis(&analysis, &preference, m_repo, their_heads, 1));

    // Check if the merge is a fast-forward and perform it if so
    if ((analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) != 0) {
        const char *log_message = "Fast-Forward Merge";
        CHECK_GIT_ERROR(git_reference_set_target(&local_ref, local_ref, remote_oid, log_message));

        // Perform the checkout to update files in the working directory
        git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
        checkout_opts.checkout_strategy =
            GIT_CHECKOUT_FORCE; // Use force if you want to ensure the working directory is clean
        CHECK_GIT_ERROR(git_checkout_head(m_repo, &checkout_opts));

        git_index *index = nullptr;
        // Update the index to match the working directory to ensure no staged files
        CHECK_GIT_ERROR(git_repository_index(&index, m_repo));
        CHECK_GIT_ERROR(git_index_read(index, true));
        CHECK_GIT_ERROR(git_index_write(index));
        git_index_free(index);
    } else if ((analysis & GIT_MERGE_ANALYSIS_NORMAL) != 0) {
        // Merge is required, handle accordingly (perhaps merge commits or a rebase)
    } else {
        // No merge is necessary, or a user intervention is required
    }

    // Clean up
    git_annotated_commit_free(remote_annotated);
    git_reference_free(local_ref);
    git_reference_free(remote_ref);
    git_remote_free(remote);
}

} // namespace caps_log::utils
