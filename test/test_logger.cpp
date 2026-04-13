#include "test_common.h"
#include <TinyLogger/logger.h>
#include <TinyLogger/types.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace TinyLogger;
using namespace TinyLogger::test;

// ==================== Logger 初始化和销毁测试 ====================

bool test_logger_init_valid_config() {
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

    TempConfigFile config("init.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    logger.shutdown();
    return true;
}

bool test_logger_init_invalid_config() {
    Logger logger;
    return !logger.init("nonexistent.json");
}

bool test_logger_init_file_printer() {
    TempLogFile log_file("output.log");

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Debug",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("file.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    logger.shutdown();
    return true;
}

// ==================== Logger 日志记录测试 ====================

bool test_logger_info() {
    TempLogFile log_file("info.log");

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("info.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    logger.info("Test info message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    std::string content = log_file.read_content();
    return content.find("Test info message") != std::string::npos;
}

bool test_logger_debug() {
    TempLogFile log_file("debug.log");

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Debug",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("debug.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    logger.debug("Test debug message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    std::string content = log_file.read_content();
    return content.find("Test debug message") != std::string::npos;
}

bool test_logger_error() {
    TempLogFile log_file("error.log");

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Error",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("error.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    logger.error("Test error message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    std::string content = log_file.read_content();
    return content.find("Test error message") != std::string::npos;
}

bool test_logger_fatal() {
    TempLogFile log_file("fatal.log");

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Fatal",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("fatal.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    logger.fatal("Test fatal message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    std::string content = log_file.read_content();
    return content.find("Test fatal message") != std::string::npos;
}

bool test_logger_formatted_output() {
    TempLogFile log_file("format.log");

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("format.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    int value = 42;
    const char* str = "test";
    logger.info("Value: {}, String: {}", value, str);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    std::string content = log_file.read_content();
    return content.find("Value: 42") != std::string::npos && content.find("String: test") != std::string::npos;
}

// ==================== Logger 级别过滤测试 ====================

bool test_logger_level_filtering() {
    TempLogFile log_file("filter.log");

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Error",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("filter.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    logger.debug("Debug message");
    logger.info("Info message");
    logger.error("Error message");
    logger.fatal("Fatal message");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    std::string content = log_file.read_content();

    if (content.find("Debug message") != std::string::npos || content.find("Info message") != std::string::npos) {
        return false;
    }

    if (content.find("Error message") == std::string::npos || content.find("Fatal message") == std::string::npos) {
        return false;
    }

    return true;
}

// ==================== Logger 并发测试 ====================

bool test_logger_concurrent_logging() {
    TempLogFile log_file("concurrent.log");

    std::string json = R"({
        "buffer_size": 1024,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 10
            }
        ]
    })";

    TempConfigFile config("concurrent.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
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

    std::string content = log_file.read_content();
    return content.find("Thread 0 Message 0") != std::string::npos;
}

// ==================== Logger 溢出策略测试 ====================

bool test_logger_overflow_discard() {
    TempLogFile log_file("overflow.log");

    std::string json = R"({
        "buffer_size": 16,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 100
            }
        ]
    })";

    TempConfigFile config("overflow.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    for (int i = 0; i < 1000; ++i) {
        logger.info("Overflow test message {}", i);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    logger.shutdown();

    std::string content = log_file.read_content();
    return !content.empty();
}

// ==================== Logger 多 Printer 测试 ====================

bool test_logger_multiple_printers() {
    TempLogFile log_file("multi_printer.log");

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
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("multi.json", json);

    Logger logger;
    if (!logger.init(config.path())) {
        return false;
    }

    logger.info("Multi-printer test");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    logger.shutdown();

    std::string content = log_file.read_content();
    return content.find("Multi-printer test") != std::string::npos;
}

// ==================== Logger 生命周期测试 ====================

bool test_logger_start_stop_cycle() {
    TempLogFile log_file("cycle.log");

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info",
                "path": ")" +
                       log_file.path() + R"(",
                "flush_every": 1
            }
        ]
    })";

    TempConfigFile config("cycle.json", json);

    for (int i = 0; i < 3; ++i) {
        Logger logger;
        if (!logger.init(config.path())) {
            return false;
        }

        logger.info("Cycle {}", i);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        logger.shutdown();
    }

    std::string content = log_file.read_content();

    for (int i = 0; i < 3; ++i) {
        std::string msg = "Cycle " + std::to_string(i);
        if (content.find(msg) == std::string::npos) {
            return false;
        }
    }

    return true;
}

// ==================== 主函数 ====================

int main() {
    TestResult result;

    run_test("Logger init with valid config", test_logger_init_valid_config, result);
    run_test("Logger init with invalid config", test_logger_init_invalid_config, result);
    run_test("Logger init with file printer", test_logger_init_file_printer, result);

    run_test("Logger info()", test_logger_info, result);
    run_test("Logger debug()", test_logger_debug, result);
    run_test("Logger error()", test_logger_error, result);
    run_test("Logger fatal()", test_logger_fatal, result);
    run_test("Logger formatted output", test_logger_formatted_output, result);

    run_test("Logger level filtering", test_logger_level_filtering, result);

    run_test("Logger concurrent logging", test_logger_concurrent_logging, result);

    run_test("Logger overflow policy (discard)", test_logger_overflow_discard, result);

    run_test("Logger with multiple printers", test_logger_multiple_printers, result);

    run_test("Logger start/stop cycle", test_logger_start_stop_cycle, result);

    print_test_summary("Logger Integration Test Suite", result);

    return result.failed > 0 ? 1 : 0;
}
