#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <coroutine>
#include <iostream>
#include <fstream>
#include <functional>

class LogManager {
public:
    static LogManager& GetInstance() {
        static LogManager instance;
        return instance;
    }

    void Start() {
        is_running_.store(true);
        log_thread_ = std::thread([this] {
            Run();
        });
    }

    void Stop() {
        is_running_.store(false);
        cond_var_.notify_all();
        if (log_thread_.joinable()) {
            log_thread_.join();
        }
    }

    void AddLogTask(std::coroutine_handle<> handle, const std::string& log_message) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.emplace(handle, log_message);
        }
        cond_var_.notify_one();
    }

private:
    LogManager() = default;
    ~LogManager() {
        Stop();
    }

    void Run() {
        while (is_running_.load() || !task_queue_.empty()) {
            std::pair<std::coroutine_handle<>, std::string> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cond_var_.wait(lock, [this] {
                    return !task_queue_.empty() || !is_running_.load();
                });

                if (!task_queue_.empty()) {
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }
            }

            if (task.first) {
                // 在日志线程中 resume 协程
                std::ofstream log_file("logs.txt", std::ios::app);
                if (log_file.is_open()) {
                    log_file << task.second << std::endl;
                    log_file.close();
                }
                task.first.resume();
            }
        }
    }

    std::atomic<bool> is_running_{false};
    std::thread log_thread_;
    std::mutex queue_mutex_;
    std::condition_variable cond_var_;
    std::queue<std::pair<std::coroutine_handle<>, std::string>> task_queue_;
};

#endif // LOG_MANAGER_H

