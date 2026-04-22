#include "test_common.h"
#include <TinyLogger/logger_builder.h>
#include <TinyLogger/types.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace tiny_logger;
using namespace tiny_logger::test;

static LoggerRef create_console_logger(LogLevel level = LogLevel::Debug) {
    return LoggerBuilder()
        .set_buffer_size(256)
        .set_overflow_policy(OverflowPolicy::Discard)
        .add_console_printer(level)
        .build_shared();
}

static LoggerRef create_file_logger(const std::string& path, LogLevel level = LogLevel::Debug, size_t flush_every = 1) {
    PrinterConfig pc;
    pc.type = PrinterType::File;
    pc.min_level = level;
    pc.file_path = path;
    pc.flush_every = flush_every;
    LoggerConfig cfg;
    cfg.buffer_size = 256;
    cfg.overflow_policy = OverflowPolicy::Discard;
    cfg.printers.push_back(pc);
    return LoggerBuilder().set_config(cfg).build_shared();
}

// ==================== Logger 初始化测试 ====================

bool test_logger_init_programmatic() {
    auto logger = create_console_logger();
    return true;
}

bool test_logger_init_file_printer() {
    TempLogFile log_file("output.log");
    auto logger = create_file_logger(log_file.path());
    return true;
}

// ==================== Logger 日志记录测试 ====================

bool test_logger_info() {
    TempLogFile log_file("info.log");
    auto logger = create_file_logger(log_file.path(), LogLevel::Info);
    logger.info("Test info message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::string content = log_file.read_content();
    return content.find("Test info message") != std::string::npos;
}

bool test_logger_debug() {
    TempLogFile log_file("debug.log");
    auto logger = create_file_logger(log_file.path(), LogLevel::Debug);
    logger.debug("Test debug message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::string content = log_file.read_content();
    return content.find("Test debug message") != std::string::npos;
}

bool test_logger_error() {
    TempLogFile log_file("error.log");
    auto logger = create_file_logger(log_file.path(), LogLevel::Error);
    logger.error("Test error message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::string content = log_file.read_content();
    return content.find("Test error message") != std::string::npos;
}

bool test_logger_fatal() {
    TempLogFile log_file("fatal.log");
    auto logger = create_file_logger(log_file.path(), LogLevel::Fatal);
    logger.fatal("Test fatal message");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::string content = log_file.read_content();
    return content.find("Test fatal message") != std::string::npos;
}

bool test_logger_formatted_output() {
    TempLogFile log_file("format.log");
    auto logger = create_file_logger(log_file.path(), LogLevel::Info);
    int value = 42;
    const char* str = "test";
    logger.info("Value: {}, String: {}", value, str);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::string content = log_file.read_content();
    return content.find("Value: 42") != std::string::npos && content.find("String: test") != std::string::npos;
}

// ==================== Logger 级别过滤测试 ====================

bool test_logger_level_filtering() {
    TempLogFile log_file("filter.log");
    auto logger = create_file_logger(log_file.path(), LogLevel::Error);

    logger.debug("Debug message");
    logger.info("Info message");
    logger.error("Error message");
    logger.fatal("Fatal message");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    auto logger = LoggerBuilder()
                      .set_buffer_size(1024)
                      .set_overflow_policy(OverflowPolicy::Discard)
                      .add_file_printer(log_file.path(), LogLevel::Info)
                      .build_shared();

    constexpr int THREAD_COUNT = 4;
    constexpr int MSGS_PER_THREAD = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([logger, t]() mutable {
            for (int i = 0; i < MSGS_PER_THREAD; ++i) {
                logger.info("Thread {} Message {}", t, i);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::string content = log_file.read_content();
    auto found = content.find("Thread 0 Message 0");
    if (found == std::string::npos) {
        std::cerr << "Log content (first 500 chars): " << content.substr(0, 500) << std::endl;
        return false;
    }
    return true;
}

// ==================== Logger 溢出策略测试 ====================

bool test_logger_overflow_discard() {
    TempLogFile log_file("overflow.log");

    {
        auto logger = LoggerBuilder()
                          .set_buffer_size(16)
                          .set_overflow_policy(OverflowPolicy::Discard)
                          .add_file_printer(log_file.path(), LogLevel::Info)
                          .build_shared();

        for (int i = 0; i < 1000; ++i) {
            logger.info("Overflow test message {}", i);
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string content = log_file.read_content();
    return !content.empty();
}

// ==================== Logger 多 Printer 测试 ====================

bool test_logger_multiple_printers() {
    TempLogFile log_file("multi_printer.log");
    {
        auto logger = LoggerBuilder()
                          .set_buffer_size(256)
                          .set_overflow_policy(OverflowPolicy::Discard)
                          .add_console_printer(LogLevel::Info)
                          .add_file_printer(log_file.path(), LogLevel::Info)
                          .build_shared();

        logger.info("Multi-printer test");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::string content = log_file.read_content();
    return content.find("Multi-printer test") != std::string::npos;
}

// ==================== Logger 生命周期测试 ====================

bool test_logger_start_stop_cycle() {
    TempLogFile log_file("cycle.log");

    for (int i = 0; i < 3; ++i) {
        auto logger = create_file_logger(log_file.path(), LogLevel::Info);
        logger.info("Cycle {}", i);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

// ==================== 错误处理测试 ====================

bool test_logger_dropped_count() {
    TempLogFile log_file("dropped.log");

    auto logger = LoggerBuilder()
                      .set_buffer_size(16)
                      .set_overflow_policy(OverflowPolicy::Discard)
                      .add_file_printer(log_file.path(), LogLevel::Info)
                      .build_shared();

    for (int i = 0; i < 1000; ++i) {
        logger.info("Overflow test message {}", i);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    return logger.dropped_count() > 0;
}

bool test_logger_init_success_no_error() {
    auto logger = create_console_logger(LogLevel::Info);
    return true;
}

// ==================== 主函数 ====================

int main() {
    TestResult result;

    run_test("Logger init with programmatic config", test_logger_init_programmatic, result);
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

    run_test("Logger dropped count", test_logger_dropped_count, result);
    run_test("Logger init success returns no error", test_logger_init_success_no_error, result);

    print_test_summary("Logger Integration Test Suite", result);

    return result.failed > 0 ? 1 : 0;
}
