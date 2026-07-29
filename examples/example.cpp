/**
 * TinyLogger 示例程序
 *
 * 编译方式（Linux）：
 *   g++ -std=c++17 -I../include -o example example.cpp -L../build -lTinyLogger -lfmt
 *
 * 或者使用 CMake 构建（在项目根目录执行）：
 *   mkdir build && cd build
 *   cmake .. && make
 *   ./outputs/example
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <tiny_logger/logger_builder.hpp>

void worker_thread(int id, tiny_logger::LoggerRef logger) {
    std::string name = "Worker-" + std::to_string(id);
    tiny_logger::set_thread_name(name.c_str());

    logger.info("工作线程 {} 启动", id);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    logger.debug("工作线程 {} 正在处理任务", id);
}

int main() {
    tiny_logger::set_thread_name("MainThread");

    auto logger = tiny_logger::LoggerBuilder()
                      .set_buffer_size(256)
                      .set_overflow_policy(tiny_logger::OverflowPolicy::Discard)
                      .add_console_printer(tiny_logger::LogLevel::Debug)
                      .build();

    logger.info("TinyLogger 初始化完成");

    constexpr int kWorkerCount = 3;
    std::vector<std::thread> workers;
    for (int i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back(worker_thread, i + 1, logger);
    }

    logger.debug("这是一条调试信息，编号：{}", 43);
    logger.error("发生了一个错误：{}", "连接超时");
    logger.fatal("严重错误：系统崩溃，错误码：{}", 0xDEAD);

    int user_id = 1001;
    logger.info("用户 {} 执行了 {} 操作", user_id, "登录");

    double pi = 3.14159265;
    logger.debug("圆周率 Pi ≈ {:.4f}", pi);

    for (auto& w : workers) {
        w.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "示例程序执行完毕" << std::endl;
    return 0;
}
