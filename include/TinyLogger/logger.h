#pragma once

#include "TinyLogger/config.h"
#include "TinyLogger/distributor.h"
#include "TinyLogger/printer_console.h"
#include "TinyLogger/printer_file.h"
#include "TinyLogger/queue_registry.h"
#include "TinyLogger/ring_buffer.h"
#include "TinyLogger/types.h"
#include <atomic>
#include <cstdio>
#include <fmt/format.h>
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

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    Logger(Logger&&) noexcept = delete;
    Logger& operator=(Logger&&) noexcept = delete;

    void init();
    void init(const LoggerConfig& config);

    void shutdown();

    size_t dropped_count() const {
        return dropped_.load(std::memory_order_relaxed);
    }

    bool set_printer_min_level(PrinterType type, LogLevel level);

    template <typename... Args>
    void log(LogLevel lvl, const char* fmt, Args&&... args);

private:
    void handle_overflow();

    RingBuffer* get_queue();
    RingBuffer* create_and_register_queue();

    template <typename... Args>
    LogEvent build_event(LogLevel lvl, const char* fmt, Args&&... args);

private:
    size_t buffer_size_{0};
    OverflowPolicy overflow_policy_;

    std::vector<std::unique_ptr<RingBuffer>> owned_queues_;
    QueueRegistry registry_;

    std::unique_ptr<Distributor> distributor_;

    std::atomic<uint64_t> dropped_{0};

    uint64_t instance_id_{0};
    static std::atomic<uint64_t> next_instance_id_;
};

template <typename... Args>
using DecayedTuple = std::tuple<std::decay_t<Args>...>;

template <typename Tuple>
constexpr size_t tuple_size_bytes() {
    return sizeof(Tuple);
}

template <typename... Args>
constexpr bool fits_in_storage() {
    using Tuple = std::tuple<std::decay_t<Args>...>;
    constexpr size_t tuple_size = sizeof(Tuple);
    return tuple_size > 0 && tuple_size <= LOG_ARGS_STORAGE_SIZE;
}

template <typename... Args>
void format_log_event(const LogEvent& event, fmt::memory_buffer& out) {
    using Tuple = DecayedTuple<Args...>;
    auto* tuple_ptr = event.storage_as<Tuple>();

    if constexpr (sizeof...(Args) == 0) {
        fmt::format_to(out, "{}", event.fmt);
    } else {
        std::apply([&](const auto&... unpacked) { fmt::format_to(out, event.fmt, unpacked...); }, *tuple_ptr);
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

template <typename... Args>
const VTable* get_vtable() {
    static const VTable vtable{format_log_event<Args...>, destroy_log_event<Args...>};
    return &vtable;
}

/**
 * @brief 创建日志事件对象
 * @tparam Args 可变模板参数
 * @param lvl 日志级别
 * @param fmt 格式化字符串
 * @param args 可变参数
 * @return 构建好的 LogEvent 对象
 */
template <typename... Args>
LogEvent Logger::build_event(LogLevel lvl, const char* fmt, Args&&... args) {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    auto tid = fast_thread_id();

    LogEvent e;
    e.level = lvl;
    e.timestamp = ts;
    e.thread_id = tid;
    e.fmt = fmt;

    using Tuple = DecayedTuple<Args...>;
    new (e.storage.data()) Tuple(std::forward<Args>(args)...);

    e.vtable = get_vtable<Args...>();
    return e;
}

template <typename... Args>
void Logger::log(LogLevel lvl, const char* fmt, Args&&... args) {
    static_assert(fits_in_storage<Args...>(),
                  "Log arguments size exceeds storage capacity (128 bytes). "
                  "Reduce argument size or use fewer arguments.");

    auto e = build_event(lvl, fmt, std::forward<Args>(args)...);

    auto* q = get_queue();
    if (!q->enqueue(std::move(e))) {
        e.destroy();
        handle_overflow();
    }
}

} // namespace tiny_logger