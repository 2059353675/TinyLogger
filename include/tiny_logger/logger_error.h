#pragma once

#include "types.h"
#include <stdexcept>
#include <string>

namespace tiny_logger {

inline std::invalid_argument invalid_buffer_size_error(size_t size) {
    return std::invalid_argument("Invalid buffer size: " + std::to_string(size) + " (must be > 0)");
}

inline std::invalid_argument invalid_overflow_policy_error(OverflowPolicy policy) {
    return std::invalid_argument("Invalid overflow policy: " + std::to_string(static_cast<int>(policy)));
}

inline std::invalid_argument invalid_printer_type_error(PrinterType type) {
    return std::invalid_argument("Invalid printer type: " + std::to_string(static_cast<int>(type)));
}

inline std::invalid_argument invalid_log_level_error(LogLevel level) {
    return std::invalid_argument("Invalid log level: " + std::to_string(static_cast<int>(level)));
}

inline std::runtime_error printer_create_error(PrinterType type) {
    std::string msg = "Failed to create printer";
    switch (type) {
        case PrinterType::Console:
            msg += ": console printer not registered";
            break;
        case PrinterType::File:
            msg += ": file printer not registered";
            break;
        case PrinterType::Null:
            msg += ": null printer not registered";
            break;
        default:
            msg += ": unknown type " + std::to_string(static_cast<int>(type));
            break;
    }
    return std::runtime_error(msg);
}

inline std::runtime_error printer_create_error(const PrinterConfig& cfg) {
    std::string msg = "Failed to create printer";
    switch (cfg.type) {
        case PrinterType::Console:
            msg += ": console printer (min_level=" + std::to_string(static_cast<int>(cfg.min_level)) + ")";
            break;
        case PrinterType::File:
            msg += ": file printer";
            if (!cfg.file_path.empty()) {
                msg += " path=\"" + cfg.file_path + "\"";
            }
            msg += " (min_level=" + std::to_string(static_cast<int>(cfg.min_level)) + ")";
            break;
        case PrinterType::Null:
            msg += ": null printer";
            break;
        default:
            msg += ": unknown type " + std::to_string(static_cast<int>(cfg.type));
            break;
    }
    return std::runtime_error(msg);
}

inline std::runtime_error unknown_error(const std::string& context) {
    return std::runtime_error("Unknown error: " + context);
}

} // namespace tiny_logger