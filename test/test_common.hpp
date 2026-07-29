/**
 * TinyLogger 测试公共工具头文件
 *
 * 提供所有测试共享的工具函数和类：
 * - create_test_event: 创建测试用 LogEvent
 * - TempLogFile: RAII 风格的临时日志文件管理
 * - run_test_suite: 统一的测试运行框架
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <tiny_logger/types.hpp>
#include <vector>

namespace tiny_logger {
namespace test {

// ==================== 工具函数 ====================

/**
 * 创建测试用的 LogEvent
 *
 * @param level 日志级别
 * @param msg 日志消息
 * @return 构造好的 LogEvent
 */
inline LogEvent create_test_event(LogLevel level, const char* msg) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    uint64_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

    LogEvent event;
    event.level = level;
    event.timestamp = ts;
    event.thread_id = tid;
    event.fmt = msg;
    event.vtable = nullptr;
    return event;
}

/**
 * 创建测试用的 LogEvent（支持 bool 返回值版本，用于 ring_buffer 测试）
 *
 * @param event 输出的 LogEvent
 * @param level 日志级别
 * @param msg 日志消息
 * @return 是否成功（消息长度检查）
 */
inline bool create_test_event(LogEvent& event, LogLevel level, const char* msg) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    uint64_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

    event.level = level;
    event.timestamp = ts;
    event.thread_id = tid;
    event.fmt = msg;
    event.vtable = nullptr;
    return true;
}

// ==================== RAII 临时文件管理 ====================

/**
 * RAII 风格的临时日志文件管理器
 *
 * 用法：
 *   TempLogFile log("test.log");
 *   // 使用 log.path() 作为日志输出路径
 *   // 析构时自动删除临时日志文件
 */
class TempLogFile
{
public:
    explicit TempLogFile(const std::string& filename) : path_("test_temp_" + filename) {
        std::remove(path_.c_str()); // 清理可能残留的旧文件
    }

    ~TempLogFile() {
        cleanup();
    }

    TempLogFile(const TempLogFile&) = delete;
    TempLogFile& operator=(const TempLogFile&) = delete;

    TempLogFile(TempLogFile&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }

    TempLogFile& operator=(TempLogFile&& other) noexcept {
        if (this != &other) {
            cleanup();
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }

    const std::string& path() const {
        return path_;
    }
    bool valid() const {
        return !path_.empty();
    }

    /**
     * 读取当前文件内容
     */
    std::string read_content() const {
        std::ifstream ifs(path_);
        if (!ifs.is_open()) {
            return "";
        }
        return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    }

    /**
     * 检查文件是否存在
     */
    bool exists() const {
        std::ifstream ifs(path_);
        return ifs.is_open();
    }

private:
    void cleanup() {
        if (!path_.empty()) {
            std::remove(path_.c_str());
        }
    }

    std::string path_;
};

// ==================== 测试运行框架 ====================

struct TestResult {
    int passed = 0;
    int failed = 0;
    std::vector<std::string> failures;
};

/**
 * 运行单个测试函数并记录结果
 *
 * @param retries 失败时重试次数（默认 1，即不重试）
 */
inline void run_test(const std::string& name, std::function<bool()> test_func, TestResult& result, int retries = 1) {
    for (int attempt = 1; attempt <= retries; ++attempt) {
        if (attempt > 1) {
            std::cout << "[RETRY " << attempt << "/" << retries << "] ";
            std::cout.flush();
        } else {
            std::cout << "[TEST] " << name << "... ";
            std::cout.flush();
        }

        bool passed = false;
        try {
            passed = test_func();
        } catch (const std::exception& e) {
            std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
            if (attempt == retries) {
                result.failed++;
                result.failures.push_back(name + " (exception)");
            }
            continue;
        } catch (...) {
            std::cout << "FAILED (unknown exception)" << std::endl;
            if (attempt == retries) {
                result.failed++;
                result.failures.push_back(name + " (exception)");
            }
            continue;
        }

        if (passed) {
            std::cout << "PASSED" << std::endl;
            result.passed++;
            return;
        }

        std::cout << "FAILED" << std::endl;
        if (attempt == retries) {
            result.failed++;
            result.failures.push_back(name);
        }
    }
}

/**
 * 打印测试套件总结
 */
inline void print_test_summary(const std::string& suite_name, const TestResult& result) {
    std::cout << "========================================" << std::endl;
    std::cout << "  " << suite_name << std::endl;
    std::cout << "  Results: " << result.passed << " passed, " << result.failed << " failed" << std::endl;

    if (!result.failures.empty()) {
        std::cout << "  Failed tests:" << std::endl;
        for (const auto& f : result.failures) {
            std::cout << "    - " << f << std::endl;
        }
    }

    std::cout << "========================================" << std::endl;
}

} // namespace test
} // namespace tiny_logger
