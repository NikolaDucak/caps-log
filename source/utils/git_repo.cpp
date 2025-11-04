#include "git_repo.hpp"
#include <fmt/format.h>
#include <string>

#include <algorithm>
#include <utility>

namespace caps_log::utils {

namespace {

void checkGitError(int code, int line, const char *file) {
    if (code != 0) {
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
    git_config *liveConfig = nullptr;
    CHECK_GIT_ERROR(git_repository_config(&liveConfig, repo));
    git_config *config = nullptr;
    git_config_snapshot(&config, liveConfig);

    // Retrieve user.name
    const char *configName = nullptr;
    CHECK_GIT_ERROR(git_config_get_string(&configName, config, "user.name") != 0);

    // Retrieve user.email
    const char *configEmail = nullptr;
    CHECK_GIT_ERROR(git_config_get_string(&configEmail, config, "user.email") != 0);

    if (not validateStr(configName)) {
        throw std::runtime_error{std::string{"Invalid name from config: "} + configName};
    }
    if (not validateStr(configEmail)) {
        throw std::runtime_error{std::string{"Invalid email from config: "} + configEmail};
    }

    // Create signature
    git_signature *sig = nullptr;
    CHECK_GIT_ERROR(git_signature_now(&sig, configName, configEmail));
    git_config_free(config);

    return sig;
}

bool hasChangedFiles(git_repository *repo) {
    git_status_list *status = NULL;
    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
                 GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    CHECK_GIT_ERROR(git_status_list_new(&status, repo, &opts));

    size_t statusCount = git_status_list_entrycount(status);
    git_status_list_free(status);

    return statusCount > 0;
}

} // namespace

GitRepo::GitRepo(GitRepoConfig config)
    : m_gitLibRaii{getGitLibRaii()}, m_mainBranchName{std::move(config.mainBranchName)},
      m_remoteName{std::move(config.remoteName)}, m_sshKeyPath{std::move(config.sshKeyPath)},
      m_sshPubKeyPath{std::move(config.sshPubKeyPath)} {

    if (m_sshKeyPath.empty() || m_sshPubKeyPath.empty()) {
        throw std::invalid_argument{"SSH keys are not set!"};
    }
    if (not std::filesystem::exists(m_sshKeyPath)) {
        throw std::invalid_argument{"SSH key does not exist: " + m_sshKeyPath};
    }
    if (not std::filesystem::exists(m_sshPubKeyPath)) {
        throw std::invalid_argument{std::string{"SSH public key does not exist: "} +
                                    m_sshPubKeyPath};
    }
    if (m_remoteName.empty()) {
        throw std::invalid_argument{"Remote name can not be empty!"};
    }
    if (m_mainBranchName.empty()) {
        throw std::invalid_argument{"Main branch name can not be empty!"};
    }
    git_libgit2_init();
    CHECK_GIT_ERROR(git_repository_open(&m_repo, config.root.c_str()));
}

GitRepo::GitRepo(GitRepo &&other) noexcept
    : m_gitLibRaii(std::move(other.m_gitLibRaii)), m_repo(other.m_repo),
      m_mainBranchName(std::move(other.m_mainBranchName)),
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

    auto pushRef = "refs/heads/" + m_mainBranchName + ":refs/heads/" + m_mainBranchName;
    git_remote_add_push(m_repo, m_remoteName.c_str(), pushRef.c_str());

    git_push_options pushOpts = GIT_PUSH_OPTIONS_INIT;
    const auto callbacks = getRemoteCallbacks();
    pushOpts.callbacks = callbacks;

    auto refspec = std::string{"refs/heads/"} + m_mainBranchName;
    auto *tmp = refspec.data();
    const git_strarray refspecs = {&tmp, 1};

    CHECK_GIT_ERROR(git_remote_push(remote, &refspecs, &pushOpts));
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
    git_oid treeOid;
    git_oid commitOid;
    git_tree *tree = nullptr;
    CHECK_GIT_ERROR(git_index_write_tree(&treeOid, index));
    CHECK_GIT_ERROR(git_tree_lookup(&tree, m_repo, &treeOid));

    // Commit
    git_oid parentCommitOid;
    git_commit *parentCommit = nullptr;
    if (git_reference_name_to_id(&parentCommitOid, m_repo, "HEAD") == 0) {
        CHECK_GIT_ERROR(git_commit_lookup(&parentCommit, m_repo, &parentCommitOid));
    }

    static const char *commitMessage = "caps-log auto push";
    // NOLINTNEXTLINE
    git_commit_create_v(&commitOid, m_repo, "HEAD", sig, sig, nullptr, commitMessage, tree,
                        parentCommit != nullptr ? 1 : 0, parentCommit);

    // Cleanup
    git_signature_free(sig);
    git_index_free(index);
    git_tree_free(tree);
    if (parentCommit != nullptr) {
        git_commit_free(parentCommit);
    }
    return true;
}
void GitRepo::pull() {
    git_remote *remote = nullptr;
    git_reference *localRef = nullptr;
    git_reference *remoteRef = nullptr;
    git_annotated_commit *remoteAnnotated = nullptr;

    // Look up the remote
    CHECK_GIT_ERROR(git_remote_lookup(&remote, m_repo, m_remoteName.c_str()));

    // Fetch the changes from the remote
    git_fetch_options fetchOpts = GIT_FETCH_OPTIONS_INIT;
    fetchOpts.callbacks = getRemoteCallbacks();
    CHECK_GIT_ERROR(git_remote_fetch(remote, nullptr, &fetchOpts, "Fetch"));

    // Get the reference for the remote tracking branch
    std::string remoteRefname = "refs/remotes/" + m_remoteName + "/" + m_mainBranchName;
    CHECK_GIT_ERROR(git_reference_lookup(&remoteRef, m_repo, remoteRefname.c_str()));

    // Create an annotated commit from the remote reference
    const git_oid *remoteOid = git_reference_target(remoteRef);
    CHECK_GIT_ERROR(git_annotated_commit_lookup(&remoteAnnotated, m_repo, remoteOid));

    // Get the reference for the local branch
    std::string localRefname = "refs/heads/" + m_mainBranchName;
    CHECK_GIT_ERROR(git_reference_lookup(&localRef, m_repo, localRefname.c_str()));

    // Perform merge analysis
    git_merge_analysis_t analysis;                                 // NOLINT
    git_merge_preference_t preference;                             // NOLINT
    const git_annotated_commit *their_heads[] = {remoteAnnotated}; // NOLINT
    CHECK_GIT_ERROR(git_merge_analysis(&analysis, &preference, m_repo, their_heads, 1));

    // Check if the merge is a fast-forward and perform it if so
    if ((analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) != 0) {
        const char *logMessage = "Fast-Forward Merge";
        CHECK_GIT_ERROR(git_reference_set_target(&localRef, localRef, remoteOid, logMessage));

        // Perform the checkout to update files in the working directory
        git_checkout_options checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOpts.checkout_strategy =
            GIT_CHECKOUT_FORCE; // Use force if you want to ensure the working directory is clean
        CHECK_GIT_ERROR(git_checkout_head(m_repo, &checkoutOpts));

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
    git_annotated_commit_free(remoteAnnotated);
    git_reference_free(localRef);
    git_reference_free(remoteRef);
    git_remote_free(remote);
}

} // namespace caps_log::utils
