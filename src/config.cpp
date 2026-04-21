#include "TinyLogger/config.h"
#include <algorithm>
#include <cctype>
#include <fstream>

namespace tiny_logger {

std::optional<LogLevel> string_to_level(std::string s) {
    if (s == "Debug")
        return LogLevel::Debug;
    if (s == "Info")
        return LogLevel::Info;
    if (s == "Error")
        return LogLevel::Error;
    if (s == "Fatal")
        return LogLevel::Fatal;
    return std::nullopt;
}

std::optional<PrinterType> string_to_printer_type(std::string s) {
    if (s == "Console")
        return PrinterType::Console;
    if (s == "File")
        return PrinterType::File;
    if (s == "Null")
        return PrinterType::Null;
    return std::nullopt;
}

std::optional<OverflowPolicy> string_to_overflow(std::string s) {
    if (s == "Discard")
        return OverflowPolicy::Discard;
    if (s == "Block")
        return OverflowPolicy::Block;
    if (s == "DropOldest")
        return OverflowPolicy::DropOldest;
    return std::nullopt;
}

std::optional<LoggerConfig> load_config(const std::string& path, ErrorCode& error) {
    error = ErrorCode::None;

    std::ifstream f(path);
    if (!f.is_open()) {
        error = ErrorCode::FileNotFound;
        return std::nullopt;
    }

    json j;
    try {
        f >> j;
    } catch (const json::parse_error&) {
        error = ErrorCode::ParseError;
        return std::nullopt;
    } catch (...) {
        error = ErrorCode::UnknownError;
        return std::nullopt;
    }

    LoggerConfig config;
    parse_buffer_config(j, config, error);
    if (error != ErrorCode::None)
        return std::nullopt;

    parse_overflow_policy(j, config, error);
    if (error != ErrorCode::None)
        return std::nullopt;

    parse_printers(j, config, error);
    if (error != ErrorCode::None)
        return std::nullopt;

    return config;
}

void parse_buffer_config(const json& j, LoggerConfig& config, ErrorCode& error) {
    if (!j.contains("buffer_size"))
        return;

    config.buffer_size = j["buffer_size"].get<size_t>();
    if ((config.buffer_size & (config.buffer_size - 1)) != 0) {
        error = ErrorCode::InvalidBufferSize;
    }
}

void parse_overflow_policy(const json& j, LoggerConfig& config, ErrorCode& error) {
    if (!j.contains("overflow_policy"))
        return;

    auto policy = string_to_overflow(j["overflow_policy"].get<std::string>());
    if (!policy) {
        error = ErrorCode::InvalidOverflowPolicy;
        return;
    }
    config.overflow_policy = *policy;
}

void parse_printers(const json& j, LoggerConfig& config, ErrorCode& error) {
    if (!j.contains("printers") || !j["printers"].is_array()) {
        error = ErrorCode::InvalidPrinterType;
        return;
    }

    for (const auto& pj : j["printers"]) {
        if (!pj.contains("type")) {
            error = ErrorCode::InvalidPrinterType;
            return;
        }
        PrinterConfig pc = parse_printer_config(pj, error);
        if (error != ErrorCode::None) {
            return;
        }
        config.printers.push_back(std::move(pc));
    }

    error = ErrorCode::None;
}

PrinterConfig parse_printer_config(const json& pj, ErrorCode& error) {
    PrinterConfig pc;

    auto type = string_to_printer_type(pj["type"].get<std::string>());
    if (!type) {
        error = ErrorCode::InvalidPrinterType;
        return pc;
    }
    pc.type = *type;

    if (pj.contains("level")) {
        auto lvl = string_to_level(pj["level"].get<std::string>());
        if (!lvl) {
            error = ErrorCode::InvalidLevel;
            return pc;
        }
        pc.min_level = *lvl;
    }

    pc.raw = pj;
    error = ErrorCode::None;
    return pc;
}

} // namespace tiny_logger
