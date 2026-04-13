#pragma once

#include "TinyLogger/config.h"
#include "TinyLogger/distributor.h"
#include "TinyLogger/printer_console.h"
#include "TinyLogger/printer_file.h"
#include "TinyLogger/ring_buffer.h"
#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>

namespace TinyLogger {

using ErrorCallback = std::function<void(LoggerException::Code, const std::string&)>;

inline const char* error_code_to_string(LoggerException::Code code) {
    switch (code) {
        case LoggerException::Code::InitFailed:
            return "InitFailed";
        case LoggerException::Code::PrinterCreateFailed:
            return "PrinterCreateFailed";
        case LoggerException::Code::BufferAllocFailed:
            return "BufferAllocFailed";
        case LoggerException::Code::WriteFailed:
            return "WriteFailed";
        default:
            return "Unknown";
    }
}

class Logger
{
public:
    Logger() = default;

    ~Logger() {
        shutdown();
    }

    void set_error_callback(ErrorCallback cb) {
        error_callback_ = std::move(cb);
    }

    bool init(const std::string& path) {
        static bool registered = [] {
            register_console_printer();
            register_file_printer();
            return true;
        }();

        ConfigError err;
        auto cfg = load_config(path, err);
        if (!cfg) {
            notify_error(LoggerException::Code::InitFailed, "Failed to load config: " + std::to_string(static_cast<int>(err)));
            return false;
        }
        config_ = *cfg;

        try {
            ring_buffer_ = std::make_unique<RingBuffer>(config_.buffer_size);
        } catch (const std::bad_alloc&) {
            notify_error(LoggerException::Code::BufferAllocFailed, "Failed to allocate ring buffer");
            return false;
        }

        distributor_ = std::make_unique<Distributor>(*ring_buffer_);

        for (const auto& pc : config_.printers) {
            try {
                auto printer = PrinterRegistry::instance().create(pc);
                if (!printer) {
                    notify_error(LoggerException::Code::PrinterCreateFailed,
                                 "Failed to create printer for type: " + std::to_string(static_cast<int>(pc.type)));
                    return false;
                }
                distributor_->add_printer(std::move(printer));
            } catch (const std::exception& e) {
                notify_error(LoggerException::Code::PrinterCreateFailed, e.what());
                return false;
            }
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

    size_t dropped_count() const {
        return dropped_.load(std::memory_order_relaxed);
    }

private:
    void notify_error(LoggerException::Code code, const std::string& msg) {
        if (error_callback_) {
            error_callback_(code, msg);
        } else {
            std::fprintf(stderr, "[TinyLogger Error][%s] %s\n", error_code_to_string(code), msg.c_str());
        }
    }

    template <typename... Args> void log(LogLevel lvl, const char* fmt, Args&&... args) {
        char buf[LOG_MSG_SIZE];

        auto now = std::chrono::steady_clock::now().time_since_epoch();
        uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

        try {
            auto s = fmt::format_to_n(buf, LOG_MSG_SIZE - 1, fmt, std::forward<Args>(args)...);

            size_t len = std::min(s.size, (size_t)(LOG_MSG_SIZE - 1));
            buf[len] = '\0';

            LogEvent e(lvl, ts, tid, buf, len);

            if (!ring_buffer_->enqueue(std::move(e))) {
                handle_overflow();
            }
        } catch (const fmt::format_error& e) {
            notify_error(LoggerException::Code::WriteFailed, "Format error: " + std::string(e.what()));
        } catch (const std::exception& e) { notify_error(LoggerException::Code::WriteFailed, e.what()); }
    }

    void handle_overflow() {
        switch (config_.overflow_policy) {
            case OverflowPolicy::Discard:
                dropped_.fetch_add(1, std::memory_order_relaxed);
                break;
            case OverflowPolicy::Block:
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                break;
            default:
                break;
        }
    }

private:
    LoggerConfig config_;
    ErrorCallback error_callback_;

    std::unique_ptr<RingBuffer> ring_buffer_;
    std::unique_ptr<Distributor> distributor_;

    std::atomic<uint64_t> dropped_{0};
};

} // namespace TinyLogger
