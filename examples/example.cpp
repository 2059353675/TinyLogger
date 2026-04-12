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
#include <iostream>
#include <thread>
#include <chrono>
#include <sys/stat.h>

// 尝试查找配置文件的多个可能路径
static const char* find_config_file() {
    const char* candidates[] = {
        "logger_config.json",           // 当前目录
        "examples/logger_config.json",  // 项目根目录运行时
        "../examples/logger_config.json", // build/examples 目录运行时
        nullptr
    };

    for (int i = 0; candidates[i] != nullptr; i++) {
        struct stat buffer;
        if (stat(candidates[i], &buffer) == 0) {
            return candidates[i];
        }
    }
    return nullptr;
}

int main() {
    // 方式一：通过配置文件初始化
    // 配置文件格式参见 test/config.json
    TinyLogger::Logger logger;

    // 尝试查找配置文件
    const char* config_path = find_config_file();
    if (config_path == nullptr) {
        std::cerr << "错误：找不到配置文件 logger_config.json" << std::endl;
        std::cerr << "请确保配置文件存在，或在代码中修改 config_path" << std::endl;
        return 1;
    }

    if (!logger.init(config_path)) {
        std::cerr << "警告：无法加载配置文件 " << config_path << std::endl;
        std::cerr << "请确保配置文件格式正确" << std::endl;
        return 1;
    }

    std::cout << "成功加载配置文件: " << config_path << std::endl;

    // 基本日志记录
    logger.info("TinyLogger 初始化完成");
    logger.debug("这是一条调试信息，编号：{}", 42);
    logger.error("发生了一个错误：{}", "连接超时");
    logger.fatal("严重错误：系统崩溃，错误码：{}", 0xDEAD);

    // 带格式化的日志
    int user_id = 1001;
    std::string action = "登录";
    logger.info("用户 {} 执行了 {} 操作", user_id, action);

    double pi = 3.14159265;
    logger.debug("圆周率 π ≈ {:.4f}", pi);

    // 等待日志刷写
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Logger 析构时会自动关闭分发器
    std::cout << "示例程序执行完毕" << std::endl;
    return 0;
}
