#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace caps_log::utils {

class ThreadedTaskExecutor {
  public:
    ThreadedTaskExecutor() : m_worker(&ThreadedTaskExecutor::worker, this) {}
    ~ThreadedTaskExecutor() { joinAll(); }

    ThreadedTaskExecutor(ThreadedTaskExecutor &&) = delete;
    ThreadedTaskExecutor &operator=(ThreadedTaskExecutor &&) = delete;
    ThreadedTaskExecutor(const ThreadedTaskExecutor &) = delete;
    ThreadedTaskExecutor &operator=(const ThreadedTaskExecutor &) = delete;

    void post(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_tasks.push(std::move(task));
        }
        m_condition.notify_one();
    }

    void joinAll() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_done = true;
        }
        m_condition.notify_one();
        m_worker.join();
    }

  private:
    std::thread m_worker;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_done{};

    void worker() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this] { return m_done || !m_tasks.empty(); });
                if (m_done && m_tasks.empty()) {
                    break;
                }
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
            // it is the responsiblity of the `task` to handle its own exceptions
            task();
        }
    }
};

} // namespace caps_log::utils
