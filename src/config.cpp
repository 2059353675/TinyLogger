#include "TinyLogger/config.h"
#include <algorithm>
#include <cctype>
#include <fstream>

namespace tiny_logger {

std::optional<LoggerConfig> load_config(const std::string& path, ErrorCode& error) {
    /* 加载配置文件 */
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

    /* 配置缓冲区大小 */
    if (j.contains("buffer_size")) {
        config.buffer_size = j["buffer_size"].get<size_t>();

        // 检查是否为 2 的幂
        if ((config.buffer_size & (config.buffer_size - 1)) != 0) {
            error = ErrorCode::InvalidBufferSize;
            return std::nullopt;
        }
    }

    /* 配置缓冲区溢出策略 */
    if (j.contains("overflow_policy")) {
        auto policy = string_to_overflow(j["overflow_policy"].get<std::string>());
        if (!policy) {
            error = ErrorCode::InvalidOverflowPolicy;
            return std::nullopt;
        }
        config.overflow_policy = *policy;
    }

    /* 加载打印器 */
    if (!j.contains("printers") || !j["printers"].is_array()) {
        error = ErrorCode::InvalidPrinterType;
        return std::nullopt;
    }
    for (const auto& pj : j["printers"]) {
        PrinterConfig pc;

        // type
        if (!pj.contains("type")) {
            error = ErrorCode::InvalidPrinterType;
            return std::nullopt;
        }
        auto type = string_to_printer_type(pj["type"].get<std::string>());
        if (!type) {
            error = ErrorCode::InvalidPrinterType;
            return std::nullopt;
        }
        pc.type = *type;

        // level
        if (pj.contains("level")) {
            auto lvl = string_to_level(pj["level"].get<std::string>());
            if (!lvl) {
                error = ErrorCode::InvalidLevel;
                return std::nullopt;
            }
            pc.min_level = *lvl;
        }

        // 特有字段
        pc.raw = pj;

        config.printers.push_back(std::move(pc));
    }

    error = ErrorCode::None;
    return config;
}

} // namespace tiny_logger
