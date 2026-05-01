#pragma once

#include "types.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace tiny_logger {

struct CachedTime {
    time_t last_sec = 0;
    char buf[32]; // "YYYY-MM-DD HH:MM:SS"
};

inline CachedTime& get_cached_time() {
    thread_local static CachedTime cache;
    return cache;
}

inline void format_timestamp(uint64_t ts_us, fmt::memory_buffer& buf) {
    time_t sec = ts_us / 1000000;
    auto& cache = get_cached_time();
    if (sec != cache.last_sec) {
        cache.last_sec = sec;

        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &sec);
#else
        localtime_r(&sec, &tm);
#endif

        std::strftime(cache.buf, sizeof(cache.buf), "%Y-%m-%d %H:%M:%S", &tm);
    }

    uint32_t us = ts_us % 1000000;
    fmt::format_to(std::back_inserter(buf), "{}.{:06}", cache.buf, us);
}

inline const char* level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return "Debug";
        case LogLevel::Info:
            return "Info";
        case LogLevel::Warn:
            return "Warn";
        case LogLevel::Error:
            return "Error";
        case LogLevel::Fatal:
            return "Fatal";
        default:
            return "Unknown";
    }
}

inline void format_log_line(LogEvent& event, fmt::memory_buffer& out) {
    fmt::format_to(std::back_inserter(out), "[");

    format_timestamp(event.timestamp, out);

    fmt::format_to(std::back_inserter(out), "][{}][{}] ", event.thread_id, level_to_string(event.level));

    if (event.vtable && event.vtable->format_fn) {
        event.vtable->format_fn(event, out);
    } else if (event.fmt) {
        fmt::format_to(std::back_inserter(out), "{}", event.fmt);
    }
}

class Printer
{
public:
    virtual ~Printer() = default;

    virtual void write(LogEvent& event) = 0;
    virtual void flush() = 0;

    bool should_log(LogLevel lvl) const {
        return static_cast<uint8_t>(lvl) >= static_cast<uint8_t>(min_level_);
    }

    size_t error_count() const {
        return error_count_.load(std::memory_order_relaxed);
    }
    void reset_error_count() {
        error_count_.store(0, std::memory_order_relaxed);
    }
    void increment_error_count() {
        error_count_.fetch_add(1, std::memory_order_relaxed);
    }

    LogLevel min_level() const {
        return min_level_;
    }
    PrinterType type() const {
        return type_;
    }
    void set_min_level(LogLevel lvl) {
        min_level_ = lvl;
    }

protected:
    LogLevel min_level_;
    PrinterType type_;
    std::atomic<size_t> error_count_{0};
};

using PrinterCreator = std::function<std::unique_ptr<Printer>(const PrinterConfig&)>;

class PrinterRegistry
{
public:
    static PrinterRegistry& instance() {
        static PrinterRegistry inst;
        return inst;
    }

    void register_printer(PrinterType type, PrinterCreator creator) {
        std::lock_guard<std::mutex> lock(mutex_);
        creators_[type] = std::move(creator);
    }

    std::unique_ptr<Printer> create(const PrinterConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = creators_.find(config.type);
        if (it == creators_.end()) {
            return nullptr;
        }

        return (it->second)(config);
    }

private:
    std::unordered_map<PrinterType, PrinterCreator> creators_;
    std::mutex mutex_;
};

} // namespace tiny_logger