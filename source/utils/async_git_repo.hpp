#pragma once

#include <concepts>
#include <exception>
#include <expected>
#include <functional> // std::move_only_function
#include <type_traits>
#include <utility>

#include "git_repo.hpp"
#include "task_executor.hpp"

namespace caps_log::utils {

/**
 * Async wrapper around GitRepo that serializes all repo operations by
 * posting work to a single-threaded executor. Results are delivered to
 * callbacks as std::expected<... , std::exception_ptr>.
 *
 * NOTE: This assumes ThreadedTaskExecutor runs tasks on exactly one thread
 * (or otherwise guarantees FIFO execution). Ensure the executor's lifetime
 * outlives any pending tasks posted here.
 */
class AsyncGitRepo final {
    GitRepo m_repo;
    ThreadedTaskExecutor m_exec;

    template <class Fn, class Cb> void postExpected(Fn a_task, Cb a_callback) {
        // Capture minimal state; don't capture *this* unless needed.
        auto *repo = &m_repo;

        m_exec.post([repo, task = std::move(a_task), callback = std::move(a_callback)]() mutable {
            using TaskReturnValue = std::invoke_result_t<Fn &, GitRepo &>;
            using ReturnValue = std::conditional_t<std::is_void_v<TaskReturnValue>, void,
                                                   std::decay_t<TaskReturnValue>>;

            try {
                if constexpr (std::is_void_v<ReturnValue>) {
                    std::invoke(task, *repo);
                    std::move(callback)(
                        std::expected<void, std::exception_ptr>{}); // has_value = true
                } else {
                    auto result = std::invoke(task, *repo);
                    std::move(callback)(std::expected<ReturnValue, std::exception_ptr>{
                        std::in_place, std::move(result)});
                }
            } catch (...) {
                if constexpr (std::is_void_v<ReturnValue>) {
                    std::move(callback)(std::expected<void, std::exception_ptr>{
                        std::unexpect, std::current_exception()});
                } else {
                    std::move(callback)(std::expected<ReturnValue, std::exception_ptr>{
                        std::unexpect, std::current_exception()});
                }
            }
        });
    }

  public:
    using VoidResultCB = std::function<void(std::expected<void, std::exception_ptr>)>;
    using CommitResultCB = std::function<void(std::expected<bool, std::exception_ptr>)>;

    // Construct with an owned GitRepo; executor is default-constructed here.
    explicit AsyncGitRepo(GitRepo repo) : m_repo{std::move(repo)} {}

    // Non-copyable/movable to avoid accidental use-after-move with posted lambdas.
    AsyncGitRepo(const AsyncGitRepo &) = delete;
    AsyncGitRepo &operator=(const AsyncGitRepo &) = delete;
    AsyncGitRepo(AsyncGitRepo &&) = delete;
    AsyncGitRepo &operator=(AsyncGitRepo &&) = delete;
    ~AsyncGitRepo() = default;

    void pull(VoidResultCB callback) {
        postExpected([](GitRepo &repo) { repo.pull(); }, std::move(callback));
    }

    void push(VoidResultCB callback) {
        postExpected([](GitRepo &repo) { repo.push(); }, std::move(callback));
    }

    void commitAll(CommitResultCB callback) {
        postExpected([](GitRepo &repo) { return repo.commitAll(); }, std::move(callback));
    }
};

} // namespace caps_log::utils
