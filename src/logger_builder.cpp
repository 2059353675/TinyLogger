#include "TinyLogger/logger_builder.h"

namespace tiny_logger {

LoggerBuilder& LoggerBuilder::set_buffer_size(size_t size) {
    config_.buffer_size = size;
    return *this;
}

LoggerBuilder& LoggerBuilder::set_overflow_policy(OverflowPolicy policy) {
    config_.overflow_policy = policy;
    return *this;
}

LoggerBuilder& LoggerBuilder::add_console_printer(LogLevel min_level) {
    PrinterConfig pc;
    pc.type = PrinterType::Console;
    pc.min_level = min_level;
    config_.printers.push_back(std::move(pc));
    return *this;
}

LoggerBuilder& LoggerBuilder::add_file_printer(const std::string& path, LogLevel min_level) {
    PrinterConfig pc;
    pc.type = PrinterType::File;
    pc.min_level = min_level;
    pc.file_path = path;
    config_.printers.push_back(std::move(pc));
    return *this;
}

LoggerBuilder& LoggerBuilder::add_printer(PrinterType type, LogLevel min_level) {
    PrinterConfig pc;
    pc.type = type;
    pc.min_level = min_level;
    config_.printers.push_back(std::move(pc));
    return *this;
}

LoggerRef LoggerBuilder::build_shared() {
    auto logger = std::make_shared<Logger>();
    logger->init(config_);
    return LoggerRef(std::move(logger));
}

const LoggerConfig& LoggerBuilder::config() const noexcept {
    return config_;
}

LoggerBuilder& LoggerBuilder::set_config(const LoggerConfig& cfg) {
    config_ = cfg;
    return *this;
}

} // namespace tiny_logger
