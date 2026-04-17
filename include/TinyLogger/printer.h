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

inline thread_local CachedTime cache;

inline std::string format_timestamp(uint64_t ts_us) {
    time_t sec = ts_us / 1000000;
    if (sec != cache.last_sec) {
        cache.last_sec = sec;

        std::tm tm;
        localtime_r(&sec, &tm);

        std::strftime(cache.buf, sizeof(cache.buf), "%Y-%m-%d %H:%M:%S", &tm);
    }

    char out[64];
    uint32_t us = ts_us % 1000000;
    int len = std::snprintf(out, sizeof(out), "%s.%06u", cache.buf, us);

    return std::string(out, len);
}

inline const char* level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return "Debug";
        case LogLevel::Info:
            return "Info";
        case LogLevel::Error:
            return "Error";
        case LogLevel::Fatal:
            return "Fatal";
        default:
            return "Unknown";
    }
}

class Printer
{
public:
    virtual ~Printer() = default;

    virtual void write(const std::string& formatted, const LogEvent& event) = 0;
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

protected:
    LogLevel min_level_;
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