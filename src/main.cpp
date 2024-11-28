#include "LogStream.h"
#include "LogWorker.h"
#include <thread>

void WorkerFunction(int id) {
    for (int i = 0; i < 1000000; ++i) {
        LOG("INFO") << "Worker " << id << " is logging message " << i;
    }
}

int main() {
    // 启动日志线程
    LogWorker::GetInstance().start();

    // 启动多个线程测试日志
    std::thread worker1(WorkerFunction, 1);
    std::thread worker2(WorkerFunction, 2);

    worker1.join();
    worker2.join();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));  // 睡眠等待
    }

    // 停止日志线程
    LogWorker::GetInstance().stop();

    return 0;
}

