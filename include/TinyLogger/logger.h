#pragma once

#include "TinyLogger/config.h"
#include "TinyLogger/distributor.h"
#include "TinyLogger/printer_console.h"
#include "TinyLogger/printer_file.h"
#include "TinyLogger/ring_buffer.h"
#include <atomic>
#include <cstdio>
#include <memory>

namespace tiny_logger {

class Logger
{
public:
    Logger();

    ~Logger() {
        shutdown();
    }

    ErrorCode init();
    ErrorCode init(const std::string& path);
    ErrorCode init(const LoggerConfig& config);

    void shutdown();

    template <typename... Args>
    void info(const char* fmt, Args&&... args) {
        log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(const char* fmt, Args&&... args) {
        log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(const char* fmt, Args&&... args) {
        log(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(const char* fmt, Args&&... args) {
        log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
    }

    size_t dropped_count() const {
        return dropped_.load(std::memory_order_relaxed);
    }

private:
    template <typename... Args>
    void log(LogLevel lvl, const char* fmt, Args&&... args);

    void handle_overflow();

private:
    std::optional<LoggerConfig> config_;

    std::unique_ptr<RingBuffer> ring_buffer_;
    std::unique_ptr<Distributor> distributor_;

    std::atomic<uint64_t> dropped_{0};
};

template <typename... Args>
void Logger::log(LogLevel lvl, const char* fmt, Args&&... args) {
    char buf[LOG_MSG_SIZE];

    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

    auto s = fmt::format_to_n(buf, LOG_MSG_SIZE - 1, fmt, std::forward<Args>(args)...);

    size_t len = std::min(s.size, (size_t)(LOG_MSG_SIZE - 1));
    buf[len] = '\0';

    LogEvent e(lvl, ts, tid, buf, len);

    if (!ring_buffer_->enqueue(std::move(e))) {
        handle_overflow();
    }
}

} // namespace tiny_logger