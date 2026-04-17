#pragma once

#include "TinyLogger/config.h"
#include "TinyLogger/distributor.h"
#include "TinyLogger/printer_console.h"
#include "TinyLogger/printer_file.h"
#include "TinyLogger/ring_buffer.h"
#include "TinyLogger/types.h"
#include <atomic>
#include <cstdio>
#include <memory>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>

namespace tiny_logger {

inline uint64_t fast_thread_id() {
    thread_local uint64_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
    return tid;
}

template <typename T>
constexpr bool is_safe_log_type() {
    return std::is_arithmetic_v<T> || std::is_same_v<T, const char*> || std::is_same_v<T, char*>;
}

template <typename... Args>
constexpr bool all_safe_types() {
    if constexpr (sizeof...(Args) == 0) {
        return true;
    } else {
        return (is_safe_log_type<std::decay_t<Args>>() && ...);
    }
}

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

namespace detail {

template <typename Tuple>
constexpr size_t tuple_size_bytes() {
    return sizeof(Tuple);
}

template <typename... Args>
using DecayedTuple = std::tuple<std::decay_t<Args>...>;

template <typename... Args>
constexpr bool fits_in_storage() {
    using Tuple = std::tuple<std::decay_t<Args>...>;
    constexpr size_t tuple_size = sizeof(Tuple);
    return tuple_size > 0 && tuple_size <= LOG_ARGS_STORAGE_SIZE;
}

template <typename Tuple, size_t... Is>
std::string format_impl(const char* fmt_str, const Tuple& args, std::index_sequence<Is...>) {
    return fmt::format(fmt_str, std::get<Is>(args)...);
}

template <typename... Args>
std::string format_log_event(const LogEvent& event) {
    using Tuple = DecayedTuple<Args...>;
    auto* tuple_ptr = event.storage_as<Tuple>();
    if constexpr (sizeof...(Args) == 0) {
        return fmt::format("{}", event.fmt_str);
    } else {
        return format_impl(event.fmt_str.c_str(), *tuple_ptr, std::index_sequence_for<Args...>{});
    }
}

template <typename... Args>
void destroy_log_event(LogEvent& event) {
    using Tuple = DecayedTuple<Args...>;
    auto* tuple_ptr = event.storage_as<Tuple>();
    if (tuple_ptr) {
        tuple_ptr->~Tuple();
    }
}

} // namespace detail

template <typename... Args>
void Logger::log(LogLevel lvl, const char* fmt, Args&&... args) {
    static_assert(detail::fits_in_storage<Args...>(),
                  "Log arguments size exceeds storage capacity (128 bytes). "
                  "Reduce argument size or use fewer arguments.");

    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    auto tid = fast_thread_id();

    LogEvent e;
    e.level = lvl;
    e.timestamp = ts;
    e.thread_id = tid;
    e.fmt_str = fmt;
    e.length = 0;

    using Tuple = detail::DecayedTuple<Args...>;
    new (e.storage.data()) Tuple(std::forward<Args>(args)...);

    e.format_fn = detail::format_log_event<Args...>;
    e.destroy_fn = detail::destroy_log_event<Args...>;

    if (!ring_buffer_->enqueue(std::move(e))) {
        e.destroy();
        handle_overflow();
    }
}

} // namespace tiny_logger