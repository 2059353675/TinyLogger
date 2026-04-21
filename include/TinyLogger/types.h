#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace tiny_logger {

using json = nlohmann::json;

const size_t LOG_COUNT = 256;
const size_t LOG_MSG_SIZE = 512;
const size_t LOG_ARGS_STORAGE_SIZE = 128;

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
    File,
    Null
};

/* 日志事件格式化 VTable（类型擦除） */
struct LogEvent;

using FormatFn = void (*)(const LogEvent&, fmt::memory_buffer&);
using DestroyFn = void (*)(LogEvent&);

struct VTable {
    FormatFn format_fn;
    DestroyFn destroy_fn;
};

/* 日志事件 */
struct LogEvent {
    LogLevel level;
    uint64_t timestamp;
    uint64_t thread_id;

    const char* fmt;
    alignas(std::max_align_t) std::array<char, LOG_ARGS_STORAGE_SIZE> storage;

    const VTable* vtable{nullptr};

    LogEvent() = default;

    LogEvent(const LogEvent&) = delete;
    LogEvent& operator=(const LogEvent&) = delete;

    LogEvent(LogEvent&& other) noexcept {
        *this = std::move(other);
    }

    LogEvent& operator=(LogEvent&& other) noexcept {
        if (this != &other) {
            level = other.level;
            timestamp = other.timestamp;
            thread_id = other.thread_id;
            fmt = other.fmt;

            std::memcpy(storage.data(), other.storage.data(), LOG_ARGS_STORAGE_SIZE);

            vtable = other.vtable;
            other.vtable = nullptr;
        }
        return *this;
    }

    ~LogEvent() {
        destroy();
    }

    void destroy() {
        if (vtable && vtable->destroy_fn) {
            vtable->destroy_fn(*this);
            vtable = nullptr;
        }
    }

    template <typename T>
    T* storage_as() {
        return std::launder(reinterpret_cast<T*>(storage.data()));
    }

    template <typename T>
    const T* storage_as() const {
        return std::launder(reinterpret_cast<const T*>(storage.data()));
    }

    template <typename T>
    bool has_storage() const {
        return storage_as<T>() != nullptr;
    }
};

/* 缓冲区的基本单位 */
struct alignas(64) Slot {
    std::atomic<size_t> sequence{0};
    LogEvent event;
};

/* 打印器配置 */
struct PrinterConfig {
    PrinterType type;
    LogLevel min_level{LogLevel::Info};

    json raw; // 存储 printer 独有字段
};

/* 日志器配置 */
struct LoggerConfig {
    size_t buffer_size;
    OverflowPolicy overflow_policy;
    std::vector<PrinterConfig> printers;

    LoggerConfig() : buffer_size(LOG_COUNT), overflow_policy(OverflowPolicy::Discard) {
    }
};

static std::optional<LogLevel> string_to_level(std::string s);
static std::optional<PrinterType> string_to_printer_type(std::string s);
static std::optional<OverflowPolicy> string_to_overflow(std::string s);

} // namespace tiny_logger
