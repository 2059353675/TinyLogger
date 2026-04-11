#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <nlohmann/json.hpp>
#include <string>

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

struct LogEvent {
    LogLevel level;
    uint64_t timestamp;
    char buffer[LOG_MSG_SIZE];
    uint16_t length;

    LogEvent() = default;
    LogEvent(LogLevel lvl, uint64_t ts, const char* msg, size_t len) : level(lvl), timestamp(ts), length(len) {
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

} // namespace TinyLogger
