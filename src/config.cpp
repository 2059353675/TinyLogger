#include "TinyLogger/config.h"
#include <algorithm>
#include <cctype>
#include <fstream>

#define TO_LOWER_CASE(str)                                                                                                       \
    std::transform((str).begin(), (str).end(), (str).begin(), [](unsigned char c) { return std::tolower(c); })

namespace TinyLogger {

using json = nlohmann::json;

// ---------- 工具函数 ----------

static std::optional<LogLevel> parse_level(std::string s) {
    TO_LOWER_CASE(s);
    if (s == "debug")
        return LogLevel::Debug;
    if (s == "info")
        return LogLevel::Info;
    if (s == "error")
        return LogLevel::Error;
    if (s == "fatal")
        return LogLevel::Fatal;
    return std::nullopt;
}

static std::optional<PrinterType> parse_printer_type(std::string s) {
    TO_LOWER_CASE(s);
    if (s == "console")
        return PrinterType::Console;
    if (s == "file")
        return PrinterType::File;
    return std::nullopt;
}

static std::optional<OverflowPolicy> parse_overflow(std::string s) {
    TO_LOWER_CASE(s);
    if (s == "discard")
        return OverflowPolicy::Discard;
    if (s == "block")
        return OverflowPolicy::Block;
    if (s == "dropoldest")
        return OverflowPolicy::DropOldest;
    return std::nullopt;
}

// ---------- 主函数 ----------

std::optional<LoggerConfig> load_config(const std::string& path, ConfigError& error) {
    try {
        std::ifstream f(path);
        if (!f.is_open()) {
            error = ConfigError::FileNotFound;
            return std::nullopt;
        }

        json j;
        f >> j;

        LoggerConfig config;

        // ---------- 全局配置 ----------
        if (j.contains("buffer_size")) {
            config.buffer_size = j["buffer_size"].get<size_t>();

            // 强约束：必须是 2 的幂（你的 ringbuffer 依赖 mask）
            if ((config.buffer_size & (config.buffer_size - 1)) != 0) {
                error = ConfigError::UnknownError;
                return std::nullopt;
            }
        }

        if (j.contains("overflow_policy")) {
            auto policy = parse_overflow(j["overflow_policy"].get<std::string>());
            if (!policy) {
                error = ConfigError::InvalidOverflowPolicy;
                return std::nullopt;
            }
            config.overflow_policy = *policy;
        }

        // ---------- printers ----------
        if (!j.contains("printers") || !j["printers"].is_array()) {
            error = ConfigError::InvalidPrinterType;
            return std::nullopt;
        }

        for (const auto& pj : j["printers"]) {
            PrinterConfig pc;

            // type
            if (!pj.contains("type")) {
                error = ConfigError::InvalidPrinterType;
                return std::nullopt;
            }

            auto type = parse_printer_type(pj["type"].get<std::string>());
            if (!type) {
                error = ConfigError::InvalidPrinterType;
                return std::nullopt;
            }
            pc.type = *type;

            // level
            if (pj.contains("level")) {
                auto lvl = parse_level(pj["level"].get<std::string>());
                if (!lvl) {
                    error = ConfigError::InvalidLevel;
                    return std::nullopt;
                }
                pc.min_level = *lvl;
            }

            // file 特有字段
            if (pc.type == PrinterType::File) {
                if (!pj.contains("path")) {
                    error = ConfigError::InvalidPrinterType;
                    return std::nullopt;
                }
                pc.file_path = pj["path"].get<std::string>();

                if (pj.contains("max_size")) {
                    pc.max_size = pj["max_size"].get<size_t>();
                }

                if (pj.contains("flush_every")) {
                    pc.flush_every = pj["flush_every"].get<size_t>();
                }
            }

            config.printers.push_back(std::move(pc));
        }

        error = ConfigError::None;
        return config;

    } catch (const json::parse_error&) {
        error = ConfigError::ParseError;
        return std::nullopt;
    } catch (...) {
        error = ConfigError::UnknownError;
        return std::nullopt;
    }
}

} // namespace TinyLogger
