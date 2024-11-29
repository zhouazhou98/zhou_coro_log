#ifndef LOG_WORKER_H
#define LOG_WORKER_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
#include <fstream>
// #include <chrono>

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

    void submit(const std::string& log_message) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        buffer_.push_back(log_message);
        current_size_ += log_message.size();
        if (current_size_ >= batch_size_) {
            cond_var_.notify_one();
        }
    }

private:
    LogWorker() = default;
    ~LogWorker() { stop(); }

    void run() {
        // while (is_running_ || !task_queue_.empty()) {
        //     std::function<void()> task;
        //     {
        //         std::unique_lock<std::mutex> lock(queue_mutex_);
        //         cond_var_.wait(lock, [this] {
        //             return !task_queue_.empty() || !is_running_;
        //         });

        //         if (!task_queue_.empty()) {
        //             task = std::move(task_queue_.front());
        //             task_queue_.pop();
        //         }
        //     }

        //     if (task) {
        //         task(); // 执行日志任务
        //     }
        // }
        while (true) {
            std::vector<std::string> local_buffer;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                // cond_var_.wait(lock, [this]() { return !is_running_ || current_size_ >= batch_size_; });
                cond_var_.wait_for(lock, flush_interval_, [this]() { return !is_running_ || current_size_ >= batch_size_; });

                if (!is_running_ && buffer_.empty()) {
                    break;
                }

                local_buffer.swap(buffer_);
                current_size_ = 0;
            }

            // 批量写入日志
            WriteLogs(local_buffer);
        }
    }

    void WriteLogs(const std::vector<std::string>& logs) {
        std::ofstream log_file("logs.txt", std::ios::app);
        if (!log_file.is_open()) {
            throw std::runtime_error("Failed to open log file");
        }

        for (const auto& log : logs) {
            log_file << log << std::endl;
        }
    }

    std::atomic<bool> is_running_{false};
    std::thread worker_thread_;
    std::mutex queue_mutex_;
    std::condition_variable cond_var_;
    std::queue<std::function<void()>> task_queue_;

    // batch
    std::vector<std::string> buffer_;      // 缓冲区
    size_t current_size_ = 0;              // 当前缓冲区大小
    const size_t batch_size_ = 128 * 1024; // 批量写入阈值

    // timer
    const std::chrono::milliseconds flush_interval_ = std::chrono::milliseconds(10); // 定时触发时间
};

#endif // LOG_WORKER_H

