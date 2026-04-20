#include "TinyLogger/logger.h"

namespace tiny_logger {

namespace {
bool registered = [] {
    register_console_printer();
    register_file_printer();
    return true;
}();
} // namespace

Logger::Logger() {
}

ErrorCode Logger::init() {
    PrinterConfig console_cfg;
    console_cfg.type = PrinterType::Console;
    console_cfg.min_level = LogLevel::Info;

    LoggerConfig default_cfg;
    default_cfg.printers.emplace_back(console_cfg);

    return init(default_cfg);
}

ErrorCode Logger::init(const std::string& path) {
    ErrorCode err;
    auto cfg = load_config(path, err);
    if (!cfg) {
        return err;
    }
    return init(*cfg);
}

ErrorCode Logger::init(const LoggerConfig& config) {
    config_ = config;

    ring_buffer_ = std::make_unique<RingBuffer>(config_->buffer_size);
    if (!ring_buffer_) {
        return ErrorCode::BufferAllocFailed;
    }

    distributor_ = std::make_unique<Distributor>(*ring_buffer_);

    for (const auto& pc : config_->printers) {
        auto printer = PrinterRegistry::instance().create(pc);
        if (!printer) {
            return ErrorCode::PrinterCreateFailed;
        }
        distributor_->add_printer(std::move(printer));
    }

    distributor_->start();
    return ErrorCode::None;
}

void Logger::shutdown() {
    if (distributor_) {
        distributor_->stop();
        distributor_.reset();
    }
}

void Logger::set_min_level(LogLevel lvl) {
    if (distributor_) {
        distributor_->set_min_level(lvl);
    }
}

void Logger::set_overflow_policy(OverflowPolicy policy) {
    if (config_) {
        config_->overflow_policy = policy;
    }
}

bool Logger::set_printer_min_level(PrinterType type, LogLevel level) {
    if (distributor_) {
        return distributor_->set_printer_min_level(type, level);
    }
    return false;
}

LogLevel Logger::get_min_level() const {
    if (distributor_) {
        return distributor_->min_level();
    }
    return LogLevel::Info;
}

OverflowPolicy Logger::get_overflow_policy() const {
    if (config_) {
        return config_->overflow_policy;
    }
    return OverflowPolicy::Discard;
}

void Logger::handle_overflow() {
    if (!config_) {
        return;
    }
    switch (config_->overflow_policy) {
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

} // namespace tiny_logger