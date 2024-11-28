#ifndef LOG_TASK_H
#define LOG_TASK_H

#include <coroutine>

class LogTask {
public:
    struct promise_type {
        LogTask get_return_object() {
            return LogTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() noexcept {}

        void return_void() noexcept {}
    };

    explicit LogTask(std::coroutine_handle<promise_type> handle) {}
        // : handle_(handle) {}

    ~LogTask() {
        // if (handle_) {
        //     handle_.destroy();
        // }
    }

    LogTask(const LogTask&) = delete;
    LogTask& operator=(const LogTask&) = delete;

    LogTask(LogTask&& other) noexcept { // : handle_(other.handle_) {
        // other.handle_ = nullptr;
    }

    // LogTask& operator=(LogTask&& other) noexcept {
    //     if (this != &other) {
    //         if (handle_) {
    //             handle_.destroy();
    //         }
    //         handle_ = other.handle_;
    //         other.handle_ = nullptr;
    //     }
    //     return *this;
    // }

private:
    // std::coroutine_handle<promise_type> handle_;
};

#endif // LOG_TASK_H

