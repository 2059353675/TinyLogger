#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace tiny_logger {

enum class ErrorCode {
    FileNotFound,
    ParseError,
    InvalidBufferSize,
    InvalidOverflowPolicy,
    InvalidPrinterType,
    InvalidLevel,
    BufferAllocFailed,
    PrinterCreateFailed,
    ConfigNotFound,
    UnknownError,
    None
};

/**
 * @brief 从指定路径加载日志器配置并返回详细错误信息
 * @param path 配置文件的路径
 * @param error 用于返回具体的配置错误类型
 * @return 成功时返回 LoggerConfig，失败时返回 std::nullopt
 */
std::optional<LoggerConfig> load_config(const std::string& path, ErrorCode& error);

} // namespace tiny_logger