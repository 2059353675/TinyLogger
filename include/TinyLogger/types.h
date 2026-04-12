#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <thread>

namespace TinyLogger {

using json = nlohmann::json;

const size_t LOG_COUNT = 256;
const size_t LOG_MSG_SIZE = 512;

/* 日志级别 */
enum class LogLevel : uint8_t {
    Debug,
    Info,
    Error,
    Fatal,
    Unknown
};

/* 溢出策略 */
enum class OverflowPolicy {
    Discard,   // 丢弃新日志（默认）
    Block,     // 阻塞等待（慎用）
    DropOldest // 丢弃最旧日志（复杂，暂不实现）
};

/* 输出类型 */
enum class PrinterType {
    Console,
    File
};

/* 日志事件 */
struct LogEvent {
    LogLevel level;
    uint64_t timestamp;
    uint64_t thread_id;

    char buffer[LOG_MSG_SIZE];
    uint16_t length;

    LogEvent() = default;
    LogEvent(LogLevel lvl, uint64_t ts, uint64_t tid, const char* msg, size_t len) 
        : level(lvl), timestamp(ts), thread_id(tid), length(len) {
        if (len < sizeof(buffer)) {
            std::memcpy(buffer, msg, len);
            buffer[len] = '\0';
        }
    }
};

/* 缓冲区的基本单位 */
struct alignas(64) Slot {
    std::atomic<size_t> sequence{0};
    LogEvent event;
};

static std::optional<LogLevel> string_to_level(std::string s) {
    if (s == "Debug")
        return LogLevel::Debug;
    if (s == "Info")
        return LogLevel::Info;
    if (s == "Error")
        return LogLevel::Error;
    if (s == "Fatal")
        return LogLevel::Fatal;
    return std::nullopt;
}

static std::optional<PrinterType> string_to_printer_type(std::string s) {
    if (s == "Console")
        return PrinterType::Console;
    if (s == "File")
        return PrinterType::File;
    return std::nullopt;
}

static std::optional<OverflowPolicy> string_to_overflow(std::string s) {
    if (s == "Discard")
        return OverflowPolicy::Discard;
    if (s == "Block")
        return OverflowPolicy::Block;
    if (s == "DropOldest")
        return OverflowPolicy::DropOldest;
    return std::nullopt;
}

} // namespace TinyLogger
