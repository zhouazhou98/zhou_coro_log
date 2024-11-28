#ifndef LOG_WORKER_H
#define LOG_WORKER_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>

class LogWorker {
public:
    static LogWorker& GetInstance() {
        static LogWorker instance;
        return instance;
    }

    void start() {
        is_running_ = true;
        worker_thread_ = std::thread([this] { run(); });
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            is_running_ = false;
        }
        cond_var_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    void schedule(std::function<void()> log_task) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.emplace(std::move(log_task));
        }
        cond_var_.notify_one();
    }

private:
    LogWorker() = default;
    ~LogWorker() { stop(); }

    void run() {
        while (is_running_ || !task_queue_.empty()) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cond_var_.wait(lock, [this] {
                    return !task_queue_.empty() || !is_running_;
                });

                if (!task_queue_.empty()) {
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }
            }

            if (task) {
                task(); // 执行日志任务
            }
        }
    }

    std::atomic<bool> is_running_{false};
    std::thread worker_thread_;
    std::mutex queue_mutex_;
    std::condition_variable cond_var_;
    std::queue<std::function<void()>> task_queue_;
};

#endif // LOG_WORKER_H

