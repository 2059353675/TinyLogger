#pragma once

#include "types.h"
#include <optional>
#include <string>
#include <vector>

namespace TinyLogger {

enum class ConfigError {
    FileNotFound,
    ParseError,
    InvalidBufferSize,
    InvalidOverflowPolicy,
    InvalidPrinterType,
    InvalidLevel,
    UnknownError,
    None
};

struct PrinterConfig {
    PrinterType type;
    LogLevel min_level{LogLevel::Info};

    // file printer
    std::string file_path;

    // 可选扩展
    size_t max_size{0};     // 文件滚动
    size_t flush_every{64}; // flush 策略
};

struct LoggerConfig {
    size_t buffer_size{256};
    OverflowPolicy overflow_policy{OverflowPolicy::Discard};
    std::vector<PrinterConfig> printers;
};

/**
 * @brief 从指定路径加载日志器配置并返回详细错误信息
 * @param path 配置文件的路径
 * @param error 用于返回具体的配置错误类型
 * @return 成功时返回 LoggerConfig，失败时返回 std::nullopt
 */
std::optional<LoggerConfig> load_config(const std::string& path, ConfigError& error);

} // namespace TinyLogger