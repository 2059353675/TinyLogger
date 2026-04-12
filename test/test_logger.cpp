#include <TinyLogger/logger.h>
#include <TinyLogger/types.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

using namespace TinyLogger;

// ==================== 工具函数 ====================

static std::string create_test_config(const std::string& content, const std::string& filename) {
    std::string path = "test_logger_" + filename;
    std::ofstream ofs(path);
    ofs << content;
    ofs.close();
    return path;
}

static void cleanup_test_config(const std::string& path) {
    std::remove(path.c_str());
}

static std::string read_file_content(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

// ==================== Logger 初始化和销毁测试 ====================

bool test_logger_init_valid_config() {
    std::cout << "[TEST] Logger init with valid config... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "Console",
                "level": "Debug"
            }
        ]
    })";

    std::string path = create_test_config(json, "init.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED" << std::endl;
        return false;
    }

    cleanup_test_config(path);
    logger.shutdown();

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_logger_init_invalid_config() {
    std::cout << "[TEST] Logger init with invalid config... ";

    Logger logger;
    if (logger.init("nonexistent.json")) {
        std::cout << "FAILED (should fail)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_logger_init_file_printer() {
    std::cout << "[TEST] Logger init with file printer... ";

    std::string log_file = "test_logger_output.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Debug",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "file.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED" << std::endl;
        return false;
    }

    cleanup_test_config(path);
    logger.shutdown();
    std::remove(log_file.c_str());

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Logger 日志记录测试 ====================

bool test_logger_info() {
    std::cout << "[TEST] Logger info()... ";

    std::string log_file = "test_info.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "info.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    logger.info("Test info message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待异步写入
    logger.shutdown();

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    if (content.find("Test info message") == std::string::npos) {
        std::cout << "FAILED (content: " << content << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_logger_debug() {
    std::cout << "[TEST] Logger debug()... ";

    std::string log_file = "test_debug.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Debug",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "debug.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    logger.debug("Test debug message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    if (content.find("Test debug message") == std::string::npos) {
        std::cout << "FAILED" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_logger_error() {
    std::cout << "[TEST] Logger error()... ";

    std::string log_file = "test_error.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Error",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "error.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    logger.error("Test error message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    if (content.find("Test error message") == std::string::npos) {
        std::cout << "FAILED" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_logger_fatal() {
    std::cout << "[TEST] Logger fatal()... ";

    std::string log_file = "test_fatal.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Fatal",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "fatal.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    logger.fatal("Test fatal message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    if (content.find("Test fatal message") == std::string::npos) {
        std::cout << "FAILED" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_logger_formatted_output() {
    std::cout << "[TEST] Logger formatted output... ";

    std::string log_file = "test_format.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "format.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    int value = 42;
    const char* str = "test";
    logger.info("Value: {}, String: {}", value, str);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    if (content.find("Value: 42") == std::string::npos || content.find("String: test") == std::string::npos) {
        std::cout << "FAILED (content: " << content << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Logger 级别过滤测试 ====================

bool test_logger_level_filtering() {
    std::cout << "[TEST] Logger level filtering... ";

    std::string log_file = "test_filter.log";
    std::remove(log_file.c_str());

    // 只记录 error 及以上
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Error",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "filter.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    logger.debug("Debug message");
    logger.info("Info message");
    logger.error("Error message");
    logger.fatal("Fatal message");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    // debug 和 info 应该被过滤
    if (content.find("Debug message") != std::string::npos || content.find("Info message") != std::string::npos) {
        std::cout << "FAILED (should filter debug and info)" << std::endl;
        return false;
    }

    // error 和 fatal 应该被记录
    if (content.find("Error message") == std::string::npos || content.find("Fatal message") == std::string::npos) {
        std::cout << "FAILED (should log error and fatal)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Logger 并发测试 ====================

bool test_logger_concurrent_logging() {
    std::cout << "[TEST] Logger concurrent logging... ";

    std::string log_file = "test_concurrent.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 1024,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 10
            }
        ]
    })";

    std::string path = create_test_config(json, "concurrent.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    constexpr int THREAD_COUNT = 4;
    constexpr int MSGS_PER_THREAD = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&logger, t]() {
            for (int i = 0; i < MSGS_PER_THREAD; ++i) {
                logger.info("Thread {} Message {}", t, i);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    logger.shutdown();

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    // 简单检查是否有一些消息
    if (content.find("Thread 0 Message 0") == std::string::npos) {
        std::cout << "FAILED" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Logger 溢出策略测试 ====================

bool test_logger_overflow_discard() {
    std::cout << "[TEST] Logger overflow policy (discard)... ";

    std::string log_file = "test_overflow.log";
    std::remove(log_file.c_str());

    // 小缓冲区
    std::string json = R"({
        "buffer_size": 16,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 100
            }
        ]
    })";

    std::string path = create_test_config(json, "overflow.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    // 快速写入大量日志，触发溢出
    for (int i = 0; i < 1000; ++i) {
        logger.info("Overflow test message {}", i);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    logger.shutdown();

    cleanup_test_config(path);

    // 不应该崩溃，文件应该有内容
    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    if (content.empty()) {
        std::cout << "FAILED (no content)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Logger 多 Printer 测试 ====================

bool test_logger_multiple_printers() {
    std::cout << "[TEST] Logger with multiple printers... ";

    std::string log_file = "test_multi_printer.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "Console",
                "level": "Info"
            },
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "multi.json");

    Logger logger;
    if (!logger.init(path)) {
        cleanup_test_config(path);
        std::cout << "FAILED (init)" << std::endl;
        return false;
    }

    logger.info("Multi-printer test");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    if (content.find("Multi-printer test") == std::string::npos) {
        std::cout << "FAILED" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Logger 生命周期测试 ====================

bool test_logger_start_stop_cycle() {
    std::cout << "[TEST] Logger start/stop cycle... ";

    std::string log_file = "test_cycle.log";
    std::remove(log_file.c_str());

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file + R"(",
                "flush_every": 1
            }
        ]
    })";

    std::string path = create_test_config(json, "cycle.json");

    // 多次启动/停止
    for (int i = 0; i < 3; ++i) {
        Logger logger;
        if (!logger.init(path)) {
            cleanup_test_config(path);
            std::cout << "FAILED (init " << i << ")" << std::endl;
            return false;
        }

        logger.info("Cycle {}", i);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        logger.shutdown();
    }

    cleanup_test_config(path);

    std::string content = read_file_content(log_file);
    std::remove(log_file.c_str());

    // 检查所有周期的消息
    for (int i = 0; i < 3; ++i) {
        std::string msg = "Cycle " + std::to_string(i);
        if (content.find(msg) == std::string::npos) {
            std::cout << "FAILED (missing: " << msg << ")" << std::endl;
            return false;
        }
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Logger Integration Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0;
    int failed = 0;

    auto run_test = [&](bool (*test_func)()) {
        if (test_func()) {
            passed++;
        } else {
            failed++;
        }
    };

    // 初始化测试
    run_test(test_logger_init_valid_config);
    run_test(test_logger_init_invalid_config);
    run_test(test_logger_init_file_printer);

    // 日志记录测试
    run_test(test_logger_info);
    run_test(test_logger_debug);
    run_test(test_logger_error);
    run_test(test_logger_fatal);
    run_test(test_logger_formatted_output);

    // 级别过滤测试
    run_test(test_logger_level_filtering);

    // 并发测试
    run_test(test_logger_concurrent_logging);

    // 溢出策略测试
    run_test(test_logger_overflow_discard);

    // 多 Printer 测试
    run_test(test_logger_multiple_printers);

    // 生命周期测试
    run_test(test_logger_start_stop_cycle);

    std::cout << "========================================" << std::endl;
    std::cout << "  Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed > 0 ? 1 : 0;
}
