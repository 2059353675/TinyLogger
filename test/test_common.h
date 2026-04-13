/**
 * TinyLogger 测试公共工具头文件
 *
 * 提供所有测试共享的工具函数和类：
 * - create_test_event: 创建测试用 LogEvent
 * - TempConfigFile: RAII 风格的临时配置文件管理
 * - run_test_suite: 统一的测试运行框架
 */

#pragma once

#include <TinyLogger/types.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace TinyLogger {
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
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    uint64_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

    size_t len = std::strlen(msg);
    return LogEvent(level, ts, tid, msg, len);
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
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    uint64_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

    size_t len = std::strlen(msg);
    if (len >= LOG_MSG_SIZE) {
        return false;
    }

    event = LogEvent(level, ts, tid, msg, len);
    return true;
}

// ==================== RAII 临时文件管理 ====================

/**
 * RAII 风格的临时配置文件管理器
 *
 * 用法：
 *   TempConfigFile config("test.json", json_content);
 *   auto result = load_config(config.path(), error);
 *   // 析构时自动删除临时文件
 */
class TempConfigFile
{
public:
    /**
     * 创建临时配置文件
     *
     * @param filename 文件名（会添加前缀 test_temp_）
     * @param content 文件内容
     */
    TempConfigFile(const std::string& filename, const std::string& content) : path_("test_temp_" + filename) {
        std::ofstream ofs(path_);
        if (ofs.is_open()) {
            ofs << content;
            ofs.close();
        }
    }

    ~TempConfigFile() {
        cleanup();
    }

    // 禁止拷贝
    TempConfigFile(const TempConfigFile&) = delete;
    TempConfigFile& operator=(const TempConfigFile&) = delete;

    // 允许移动
    TempConfigFile(TempConfigFile&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }

    TempConfigFile& operator=(TempConfigFile&& other) noexcept {
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

private:
    void cleanup() {
        if (!path_.empty()) {
            std::remove(path_.c_str());
        }
    }

    std::string path_;
};

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
 */
inline void run_test(const std::string& name, std::function<bool()> test_func, TestResult& result) {
    std::cout << "[TEST] " << name << "... ";
    std::cout.flush();

    try {
        if (test_func()) {
            std::cout << "PASSED" << std::endl;
            result.passed++;
        } else {
            std::cout << "FAILED" << std::endl;
            result.failed++;
            result.failures.push_back(name);
        }
    } catch (const std::exception& e) {
        std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
        result.failed++;
        result.failures.push_back(name + " (exception)");
    } catch (...) {
        std::cout << "FAILED (unknown exception)" << std::endl;
        result.failed++;
        result.failures.push_back(name + " (exception)");
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
} // namespace TinyLogger
