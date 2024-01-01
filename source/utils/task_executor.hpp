#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace caps_log::utils {

class ThreadedTaskExecutor {
  public:
    ThreadedTaskExecutor() : done(false), worker(&ThreadedTaskExecutor::Worker, this) {}
    ~ThreadedTaskExecutor() { JoinAll(); }

    ThreadedTaskExecutor(ThreadedTaskExecutor &&) = delete;
    ThreadedTaskExecutor &operator=(ThreadedTaskExecutor &&) = delete;
    ThreadedTaskExecutor(const ThreadedTaskExecutor &) = delete;
    ThreadedTaskExecutor &operator=(const ThreadedTaskExecutor &) = delete;

    void Post(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            tasks.push(std::move(task));
        }
        condition.notify_one();
    }

    void JoinAll() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            done = true;
        }
        condition.notify_one();
        worker.join();
    }

  private:
    std::thread worker;
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    bool done;

    void Worker() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [this] { return done || !tasks.empty(); });
                if (done && tasks.empty()) {
                    break;
                }
                task = std::move(tasks.front());
                tasks.pop();
            }
            try {
                task();
            } catch (const std::exception &e) {
                // Handle or log the exception as needed
            }
        }
    }
};

} // namespace caps_log::utils
