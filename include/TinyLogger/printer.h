#pragma once

#include "types.h"
#include <functional>
#include <mutex>
#include <unordered_map>

namespace TinyLogger {

inline std::string format_timestamp(uint64_t ts_us) {
    using namespace std::chrono;

    system_clock::time_point tp{microseconds(ts_us)};
    std::time_t t = system_clock::to_time_t(tp);

    std::tm tm{};
    localtime_r(&t, &tm);

    auto us = ts_us % 1000000;

    return fmt::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:06}",
                       tm.tm_year + 1900,
                       tm.tm_mon + 1,
                       tm.tm_mday,
                       tm.tm_hour,
                       tm.tm_min,
                       tm.tm_sec,
                       us);
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

    virtual void write(const LogEvent& event) = 0;
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

} // namespace TinyLogger