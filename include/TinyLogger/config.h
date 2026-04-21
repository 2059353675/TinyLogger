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

std::optional<LogLevel> string_to_level(std::string s);
std::optional<PrinterType> string_to_printer_type(std::string s);
std::optional<OverflowPolicy> string_to_overflow(std::string s);

void parse_buffer_config(const json& j, LoggerConfig& config, ErrorCode& error);
void parse_overflow_policy(const json& j, LoggerConfig& config, ErrorCode& error);
void parse_printers(const json& j, LoggerConfig& config, ErrorCode& error);
PrinterConfig parse_printer_config(const json& pj, ErrorCode& error);

} // namespace tiny_logger