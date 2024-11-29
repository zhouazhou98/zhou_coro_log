#ifndef LOG_STREAM_H
#define LOG_STREAM_H

#include "LogWorker.h"
// #include "LogTask.h"
#include <sstream>
#include <coroutine>
#include <iomanip>
#include <fstream>

// class LogAwaiter {
// public:
//     LogAwaiter(const std::string& log_message)
//         : log_message_(log_message) {}
// 
//     bool await_ready() const noexcept {
//         return false; // 始终挂起
//     }
// 
//     void await_suspend(std::coroutine_handle<> handle) noexcept {
//         // 将协程任务交给日志线程
//         LogManager::GetInstance().AddLogTask(handle, log_message_);
//     }
// 
//     void await_resume() {}
// 
// private:
//     std::string log_message_;
// };

class LogStream {
public:
    LogStream(const std::string& level, const char* file, int line)
        : level_(level), file_(file), line_(line) {
            ss_ << "[" << GetCurrentTimestamp() << "] "
            << "[" << level_ << "] "
            << "[" << file_ << ":" << line_ << "] ";
        }

    ~LogStream() {
        // RunLogTask();
        SubmitLog();
    }

    template <typename T>
    LogStream& operator<<(const T& value) {
        ss_ << value; // 写入流
        return *this;
    }

    // 重载 operator new
    void* operator new(std::size_t size);

    // 重载 operator delete
    void operator delete(void* ptr) noexcept;

private:
    std::string level_;
    const char* file_;
    int line_;
    std::ostringstream ss_;

    static std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void RunLogTask() {
        // auto log_task = 
        
        LogWorker::GetInstance().schedule([log = std::move(ss_.str())]() {
            // co_await LogAwaiter(log);
            WriteLogToFile(log);
        });
        // log_task();
    }

    void SubmitLog() {
        LogWorker::GetInstance().submit(ss_.str());
    }

    static void WriteLogToFile(const std::string& log_message) {
        std::ofstream log_file("logs.txt", std::ios::app);
        if (log_file.is_open()) {
            log_file << log_message << std::endl;
        }
    }
};


// 线程局部存储的 LogStream 指针
static thread_local LogStream* current_log_ = nullptr;

void* LogStream::operator new(std::size_t size) {
    // if (current_log_) {
    //     throw std::runtime_error("Only one LogStream instance is allowed per thread.");
    // }
    // current_log_ = static_cast<LogStream*>(::operator new(size)); // 分配内存
    // return current_log_;
    if (!current_log_) {
            current_log_ = static_cast<LogStream*>(std::malloc(size));
            if (!current_log_) {
                throw std::bad_alloc();
            }
        }
    return current_log_;
}

void LogStream::operator delete(void* ptr) noexcept {
    // if (current_log_) {
    //     ::operator delete(ptr);  // 释放内存
    //     current_log_ = nullptr;  // 标记为可用
    // }
}

#define LOG(level) if(volatile bool log_guard = true) LogStream(level, __FILE__, __LINE__)

#endif // LOG_STREAM_H

