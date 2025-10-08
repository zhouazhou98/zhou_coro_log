#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
#include <fstream>

class LogWorker2 {
public:
    static LogWorker2& GetInstance() {
        static LogWorker2 instance;
        return instance;
    }

    void start() {
        is_running_ = true;
        worker_thread_ = std::thread([this] { processLogs(); });
        disk_thread_ = std::thread([this] { run(); });
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            is_running_ = false;
        }
        cond_var_.notify_all();
        disk_cond_var_.notify_all();

        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        if (disk_thread_.joinable()) {
            disk_thread_.join();
        }
    }

    void submit(std::string&& log_message) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.emplace(std::move(log_message));
        }
        cond_var_.notify_one();
    }

private:
    LogWorker2() = default;
    ~LogWorker2() { stop(); }

// void processLogs() {
//     while (is_running_ || !task_queue_.empty()) {
//         std::unique_lock<std::mutex> lock(queue_mutex_);
//         cond_var_.wait(lock, [this] {
//             return !is_running_ || !task_queue_.empty();
//         });
// 
//         while (!task_queue_.empty()) {
//             // 确保字符串从队列中移动到缓冲区
//             auto log_message = task_queue_.front();
//             local_buffer_.push_back(log_message);
//             task_queue_.pop();
//         }
// 
//         if (local_buffer_.size() >= batch_size_) {
//             disk_cond_var_.notify_one();
//         }
//     }
// 
//     // 通知磁盘线程退出
//     disk_cond_var_.notify_one();
// }

    // void processLogs() {
    //     while (is_running_ || !task_queue_.empty()) {
    //         std::unique_lock<std::mutex> lock(queue_mutex_);
    //         cond_var_.wait(lock, [this] {
    //             return !is_running_ || !task_queue_.empty();
    //         });

    //         while (!task_queue_.empty()) {
    //             // 使用移动语义
    //             local_buffer_.push_back(std::move(task_queue_.front()));
    //             task_queue_.pop();
    //         }

    //         if (local_buffer_.size() >= batch_size_) {
    //             disk_cond_var_.notify_one();
    //         }
    //     }

    //     // 通知磁盘线程退出
    //     disk_cond_var_.notify_one();
    // }

    void processLogs() {
        while (is_running_ || !task_queue_.empty()) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cond_var_.wait(lock, [this] {
                return !is_running_ || !task_queue_.empty();
            });

            while (!task_queue_.empty()) {
                auto& log_message = task_queue_.front();
                local_buffer_.push_back(log_message);
                current_size_ += log_message.size();
                task_queue_.pop();
            }

            if (current_size_ >= batch_size_) {
                disk_cond_var_.notify_one();
            }
        }
        // 通知磁盘线程退出
        disk_cond_var_.notify_one();
    }

    void run() {
        while (is_running_ || !local_buffer_.empty()) {
            std::vector<std::string> logs_to_write;
            {
                std::unique_lock<std::mutex> lock(disk_mutex_);
                disk_cond_var_.wait_for(lock, flush_interval_, [this]() {
                    return !is_running_ || current_size_ >= batch_size_;
                });

                if (!is_running_ && local_buffer_.empty()) {
                    break;
                }

                logs_to_write.swap(local_buffer_);
                current_size_ = 0;
            }

            // 写入磁盘
            WriteLogs(logs_to_write);
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
    std::thread disk_thread_;
    std::mutex queue_mutex_;
    std::condition_variable cond_var_;
    std::queue<std::string> task_queue_; // 存储日志消息的队列

    // 用于批量处理的缓冲区
    std::vector<std::string> local_buffer_;
    size_t current_size_ = 0;              
    const size_t batch_size_ = 128 * 1024; 

    // 磁盘写入线程相关
    std::mutex disk_mutex_;
    std::condition_variable disk_cond_var_;
    const std::chrono::milliseconds flush_interval_ = std::chrono::milliseconds(10);
};