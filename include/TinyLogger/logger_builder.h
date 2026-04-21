#pragma once

#include "TinyLogger/config.h"
#include "TinyLogger/logger.h"
#include "TinyLogger/printer.h"
#include "TinyLogger/types.h"
#include <memory>
#include <string>
#include <utility>

namespace tiny_logger {

/**
 * @brief 日志器构建器
 * @details 提供链式配置接口，用于构建 Logger 实例
 */
class LoggerBuilder
{
public:
    LoggerBuilder();
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
    LoggerBuilder& set_buffer_size(size_t size) {
        config_.buffer_size = size;
        return *this;
    }

    /**
     * @brief 设置溢出策略
     * @param policy 溢出策略（默认 Discard）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& set_overflow_policy(OverflowPolicy policy) {
        config_.overflow_policy = policy;
        return *this;
    }

    /**
     * @brief 添加控制台打印机
     * @param min_level 最小日志级别（默认 Info）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& add_console_printer(LogLevel min_level = LogLevel::Info) {
        PrinterConfig pc;
        pc.type = PrinterType::Console;
        pc.min_level = min_level;
        config_.printers.push_back(std::move(pc));
        return *this;
    }

    /**
     * @brief 添加文件打印机
     * @param path 日志文件路径
     * @param min_level 最小日志级别（默认 Debug）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& add_file_printer(const std::string& path, LogLevel min_level = LogLevel::Debug) {
        PrinterConfig pc;
        pc.type = PrinterType::File;
        pc.min_level = min_level;
        pc.raw = json::object();
        pc.raw["path"] = path;
        config_.printers.push_back(std::move(pc));
        return *this;
    }

    /**
     * @brief 添加通用打印机
     * @param type 打印机类型
     * @param min_level 最小日志级别（默认 Debug）
     * @return 引用自身，支持链式调用
     */
    LoggerBuilder& add_printer(PrinterType type, LogLevel min_level = LogLevel::Debug) {
        PrinterConfig pc;
        pc.type = type;
        pc.min_level = min_level;
        config_.printers.push_back(std::move(pc));
        return *this;
    }

    /**
     * @brief 构建 Logger 实例
     * @return 初始化完成的 Logger 右值引用
     * @note 调用后 Builder 不应再被使用
     */
    Logger build() {
        Logger logger;
        auto err = logger.init(config_);
        if (err != ErrorCode::None) {
            throw std::runtime_error("Logger build failed: " + std::to_string(static_cast<int>(err)));
        }
        return logger;
    }

    /**
     * @brief 构建共享指针 Logger 实例
     * @return 共享指针
     */
    std::unique_ptr<Logger> build_shared() {
        auto logger = std::make_unique<Logger>();
        auto err = logger->init(config_);
        if (err != ErrorCode::None) {
            throw std::runtime_error("Logger build failed: " + std::to_string(static_cast<int>(err)));
        }
        return logger;
    }

    /**
     * @brief 获取内部配置（用于检查或进一步修改）
     * @return 配置引用
     */
    const LoggerConfig& config() const noexcept {
        return config_;
    }

    /**
     * @brief LoggerConfig 转换入口
     * @param cfg 已构建的配置对象
     * @return 构建器引用自身
     */
    LoggerBuilder& set_config(const LoggerConfig& cfg) {
        config_ = cfg;
        return *this;
    }

private:
    LoggerConfig config_;
};

/**
 * @brief 创建默认配置的 Logger
 * @return 带有默认控制台输出的 Logger
 */
inline Logger create_default_logger() {
    return LoggerBuilder().add_console_printer().build();
}

} // namespace tiny_logger