#pragma once

#include "TinyLogger/config.h"
#include "TinyLogger/distributor.h"
#include "TinyLogger/printer_console.h"
#include "TinyLogger/printer_file.h"
#include "TinyLogger/ring_buffer.h"
#include <atomic>
#include <memory>

namespace TinyLogger {

class Logger
{
public:
    Logger() = default;

    ~Logger() {
        shutdown();
    }

    bool init(const std::string& path) {
        // 注册 printers（仅首次生效）
        static bool registered = []{
            register_console_printer();
            register_file_printer();
            return true;
        }();

        // 读取配置
        ConfigError err;
        auto cfg = load_config(path, err);
        if (!cfg) {
            return false;
        }
        config_ = *cfg;

        // 初始化缓冲区和分发器
        ring_buffer_ = std::make_unique<RingBuffer>(config_.buffer_size);
        distributor_ = std::make_unique<Distributor>(*ring_buffer_);

        // 创建 printers
        for (const auto& pc : config_.printers) {
            auto printer = PrinterRegistry::instance().create(pc);
            if (!printer) {
                throw std::runtime_error("Failed to create printer.");
                return false;
            }
            distributor_->add_printer(std::move(printer));
        }

        distributor_->start();
        return true;
    }

    void shutdown() {
        if (distributor_) {
            distributor_->stop();
            distributor_.reset();
        }
    }

    template <typename... Args> void info(const char* fmt, Args&&... args) {
        log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args> void debug(const char* fmt, Args&&... args) {
        log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args> void error(const char* fmt, Args&&... args) {
        log(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args> void fatal(const char* fmt, Args&&... args) {
        log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
    }

private:
    template <typename... Args> void log(LogLevel lvl, const char* fmt, Args&&... args) {
        char buf[LOG_MSG_SIZE];

        // 获取时间戳
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

        // 获取线程ID
        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

        auto s = fmt::format_to_n(buf, LOG_MSG_SIZE - 1, fmt, std::forward<Args>(args)...);

        size_t len = std::min(s.size, (size_t)(LOG_MSG_SIZE - 1));
        buf[len] = '\0';

        LogEvent e(lvl, ts, tid, buf, len);

        if (!ring_buffer_->enqueue(std::move(e))) {
            handle_overflow();
        }
    }

    void handle_overflow() {
        switch (config_.overflow_policy) {
            case OverflowPolicy::Discard:
                dropped_.fetch_add(1, std::memory_order_relaxed);
                break;
            case OverflowPolicy::Block:
                // 简单阻塞（避免死循环）
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                break;
            default:
                break;
        }
    }

private:
    LoggerConfig config_;

    std::unique_ptr<RingBuffer> ring_buffer_;
    std::unique_ptr<Distributor> distributor_;

    std::atomic<uint64_t> dropped_{0};
};

} // namespace TinyLogger
