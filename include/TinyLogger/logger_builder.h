#pragma once

#include "TinyLogger/config.h"
#include "TinyLogger/logger.h"
#include "TinyLogger/printer.h"
#include "TinyLogger/types.h"
#include <memory>
#include <string>
#include <utility>

namespace tiny_logger {

class LoggerRef
{
public:
    explicit LoggerRef(std::shared_ptr<Logger> logger) : ptr_(holder_.get()), holder_(std::move(logger)) {
    }

    LoggerRef(const LoggerRef&) = default;
    LoggerRef& operator=(const LoggerRef&) = default;

    LoggerRef(LoggerRef&&) noexcept = default;
    LoggerRef& operator=(LoggerRef&&) noexcept = default;

    template <typename... Args>
    void info(const char* fmt, Args&&... args) {
        if (ptr_) {
            ptr_->log(LogLevel::Info, fmt, std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void debug(const char* fmt, Args&&... args) {
        if (ptr_) {
            ptr_->log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void error(const char* fmt, Args&&... args) {
        if (ptr_) {
            ptr_->log(LogLevel::Error, fmt, std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void fatal(const char* fmt, Args&&... args) {
        if (ptr_) {
            ptr_->log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
        }
    }

    size_t dropped_count() const {
        return ptr_ ? ptr_->dropped_count() : 0;
    }

    bool set_printer_min_level(PrinterType type, LogLevel level) {
        return ptr_ ? ptr_->set_printer_min_level(type, level) : false;
    }

private:
    std::shared_ptr<Logger> holder_;
    Logger* ptr_;
};

/**
 * @brief 日志器构建器
 * @details 提供链式配置接口，用于构建 Logger 实例
 */
class LoggerBuilder
{
public:
    LoggerBuilder() = default;
    ~LoggerBuilder() = default;

    LoggerBuilder(const LoggerBuilder&) = delete;
    LoggerBuilder& operator=(const LoggerBuilder&) = delete;

    LoggerBuilder(LoggerBuilder&&) noexcept = default;
    LoggerBuilder& operator=(LoggerBuilder&&) noexcept = default;

    /**
     * @brief 设置缓冲区大小
     * @param size 环形缓冲区大小（默认 256）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& set_buffer_size(size_t size);

    /**
     * @brief 设置溢出策略
     * @param policy 溢出策略（默认 Discard）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& set_overflow_policy(OverflowPolicy policy);

    /**
     * @brief 添加控制台打印器
     * @param min_level 最小日志级别（默认 Info）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& add_console_printer(LogLevel min_level = LogLevel::Info);

    /**
     * @brief 添加文件打印器
     * @param path 日志文件路径
     * @param min_level 最小日志级别（默认 Debug）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& add_file_printer(const std::string& path, LogLevel min_level = LogLevel::Debug);

    /**
     * @brief 添加通用打印器
     * @param type 打印器类型
     * @param min_level 最小日志级别（默认 Debug）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& add_printer(PrinterType type, LogLevel min_level = LogLevel::Debug);

    /**
     * @brief 构建 LoggerRef 实例
     * @return LoggerRef 包装类
     * @throw std::invalid_argument 参数校验失败
     * @throw std::runtime_error 打印器创建失败
     */
    LoggerRef build_shared();

    /**
     * @brief 获取内部配置（用于检查或进一步修改）
     * @return 配置引用
     */
    const LoggerConfig& config() const noexcept;

    /**
     * @brief LoggerConfig 转换入口
     * @param cfg 已构建的配置对象
     * @return 构建器引用自身
     */
    LoggerBuilder& set_config(const LoggerConfig& cfg);

private:
    LoggerConfig config_;
};

/**
 * @brief 创建默认配置的 Logger
 * @return LoggerRef 包装类
 */
inline LoggerRef create_default_logger() {
    return LoggerBuilder().add_console_printer().build_shared();
}

} // namespace tiny_logger