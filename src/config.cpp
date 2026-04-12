#include "TinyLogger/config.h"
#include <algorithm>
#include <cctype>
#include <fstream>

namespace TinyLogger {

std::optional<LoggerConfig> load_config(const std::string& path, ConfigError& error) {
    try {
        /* 加载配置文件 */
        std::ifstream f(path);
        if (!f.is_open()) {
            error = ConfigError::FileNotFound;
            return std::nullopt;
        }

        json j;
        f >> j;

        LoggerConfig config;

        /* 配置缓冲区大小 */
        if (j.contains("buffer_size")) {
            config.buffer_size = j["buffer_size"].get<size_t>();

            // 检查是否为 2 的幂
            if ((config.buffer_size & (config.buffer_size - 1)) != 0) {
                error = ConfigError::InvalidBufferSize;
                return std::nullopt;
            }
        }

        /* 配置缓冲区溢出策略 */
        if (j.contains("overflow_policy")) {
            auto policy = string_to_overflow(j["overflow_policy"].get<std::string>());
            if (!policy) {
                error = ConfigError::InvalidOverflowPolicy;
                return std::nullopt;
            }
            config.overflow_policy = *policy;
        }

        /* 加载打印器 */
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
            auto type = string_to_printer_type(pj["type"].get<std::string>());
            if (!type) {
                error = ConfigError::InvalidPrinterType;
                return std::nullopt;
            }
            pc.type = *type;

            // level
            if (pj.contains("level")) {
                auto lvl = string_to_level(pj["level"].get<std::string>());
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
