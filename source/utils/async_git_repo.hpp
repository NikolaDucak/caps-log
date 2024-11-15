#pragma once

#include "git_repo.hpp"
#include "task_executor.hpp"

namespace caps_log::utils {

/**
 * A class that has the interface of GitRepo but extends each method to provide callback
 * functionality while executing real git log repo stuff on ThreadedTaskExecutor.
 * It ensures that only one thread is touching the repo.
 */
class AsyncGitRepo final {
    GitRepo m_repo;
    ThreadedTaskExecutor m_taskExec;

  public:
    explicit AsyncGitRepo(GitRepo repo) : m_repo{std::move(repo)} {}

    void pull(std::function<void()> acallback) {
        m_taskExec.post([this, callback = std::move(acallback)] {
            m_repo.pull();
            callback();
        });
    }

    void push(std::function<void()> acallback) {
        m_taskExec.post([this, callback = std::move(acallback)]() {
            m_repo.push();
            callback();
        });
    }

    void commitAll(std::function<void(bool)> acallback) {
        m_taskExec.post([this, callback = std::move(acallback)]() {
            auto somethingCommitted = m_repo.commitAll();
            callback(somethingCommitted);
        });
    }
};

} // namespace caps_log::utils
