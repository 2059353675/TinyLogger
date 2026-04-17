/**
 * TinyLogger 示例程序
 *
 * 编译方式（Linux）：
 *   g++ -std=c++17 -I../include -o example example.cpp -L../build -lTinyLogger -lfmt
 *
 * 或者使用 CMake 构建（在项目根目录执行）：
 *   mkdir build && cd build
 *   cmake .. && make
 *   ./examples/example
 */

#include <TinyLogger/logger.h>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    // 方式一（推荐）：程序化配置 - 类型安全，无需外部文件依赖
    PrinterConfig console_cfg;
    console_cfg.type = PrinterType::Console;
    console_cfg.min_level = LogLevel::Debug;

    LoggerConfig config;
    config.buffer_size = 256;
    config.overflow_policy = OverflowPolicy::Discard;
    config.printers.push_back(console_cfg);

    Logger logger;
    auto err = logger.init(config);
    if (err != ErrorCode::None) {
        std::cerr << "初始化失败，错误码：" << static_cast<int>(err) << std::endl;
        return 1;
    }

    logger.info("TinyLogger 初始化完成");
    logger.debug("这是一条调试信息，编号：{}", 42);
    logger.error("发生了一个错误：{}", "连接超时");
    logger.fatal("严重错误：系统崩溃，错误码：{}", 0xDEAD);

    int user_id = 1001;
    std::string action = "登录";
    logger.info("用户 {} 执行了 {} 操作", user_id, action);

    double pi = 3.14159265;
    logger.debug("圆周率 π ≈ {:.4f}", pi);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Logger 析构时会自动关闭分发器
    std::cout << "示例程序执行完毕" << std::endl;
    return 0;
}
