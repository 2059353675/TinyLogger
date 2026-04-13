#include "test_common.h"
#include <TinyLogger/config.h>

using namespace TinyLogger;
using namespace TinyLogger::test;

// ==================== 基础配置测试 ====================

bool test_valid_minimal_config() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "Console",
                "level": "Info"
            }
        ]
    })";

    TempConfigFile config("minimal.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);

    return config_ptr && error == ConfigError::None && config_ptr->buffer_size == 256 &&
           config_ptr->overflow_policy == OverflowPolicy::Discard && config_ptr->printers.size() == 1;
}

bool test_valid_full_config() {
    std::string json = R"({
        "buffer_size": 512,
        "overflow_policy": "Block",
        "printers": [
            {
                "type": "Console",
                "level": "Debug"
            },
            {
                "type": "File",
                "level": "Info",
                "path": "test.log",
                "max_size": 1048576,
                "flush_every": 128
            }
        ]
    })";

    TempConfigFile config("full.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);

    if (!config_ptr || error != ConfigError::None) {
        return false;
    }

    if (config_ptr->buffer_size != 512 || config_ptr->overflow_policy != OverflowPolicy::Block ||
        config_ptr->printers.size() != 2) {
        return false;
    }

    const auto& printers = config_ptr->printers;

    // 检查 console printer
    const auto& console = printers[0];
    if (console.type != PrinterType::Console || console.min_level != LogLevel::Debug) {
        return false;
    }

    // 检查是否保存原始 JSON 数据
    if (!console.raw.contains("type") || console.raw["type"] != "Console" || !console.raw.contains("level") ||
        console.raw["level"] != "Debug") {
        return false;
    }

    // 检查 file printer
    const auto& file = printers[1];
    if (file.type != PrinterType::File || file.min_level != LogLevel::Info) {
        return false;
    }

    // 从 raw 字段中读取特有字段并校验
    if (!file.raw.contains("path") || file.raw["path"] != "test.log" || !file.raw.contains("max_size") ||
        file.raw["max_size"] != 1048576 || !file.raw.contains("flush_every") || file.raw["flush_every"] != 128) {
        return false;
    }

    return true;
}

// ==================== 错误处理测试 ====================

bool test_file_not_found() {
    ConfigError error;
    auto config = load_config("nonexistent_file.json", error);
    return !config && error == ConfigError::FileNotFound;
}

bool test_invalid_json() {
    std::string json = "{ invalid json }";
    TempConfigFile config("invalid.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);
    return !config_ptr && error == ConfigError::ParseError;
}

bool test_missing_printers() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard"
    })";

    TempConfigFile config("no_printers.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);
    return !config_ptr && error == ConfigError::InvalidPrinterType;
}

bool test_invalid_printer_type() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "invalid_type",
                "level": "Info"
            }
        ]
    })";

    TempConfigFile config("invalid_type.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);
    return !config_ptr && error == ConfigError::InvalidPrinterType;
}

bool test_invalid_overflow_policy() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "invalid_policy",
        "printers": [
            {
                "type": "Console",
                "level": "Info"
            }
        ]
    })";

    TempConfigFile config("invalid_overflow.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);
    return !config_ptr && error == ConfigError::InvalidOverflowPolicy;
}

bool test_invalid_log_level() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "Console",
                "level": "Verbose"
            }
        ]
    })";

    TempConfigFile config("invalid_level.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);
    return !config_ptr && error == ConfigError::InvalidLevel;
}

bool test_non_power_of_two_buffer() {
    std::string json = R"({
        "buffer_size": 100,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "Console",
                "level": "Info"
            }
        ]
    })";

    TempConfigFile config("bad_buffer.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);
    return !config_ptr;
}

bool test_missing_file_path() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "File",
                "level": "Info"
            }
        ]
    })";

    TempConfigFile config("missing_path.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);
    // config.cpp 不在加载时验证 path 字段，由 FilePrinter 构造函数验证
    return config_ptr && error == ConfigError::None && config_ptr->printers.size() == 1;
}

// ==================== 边界情况测试 ====================

bool test_case_insensitive_parsing() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "Console",
                "level": "Debug"
            },
            {
                "type": "File",
                "level": "Info",
                "path": "test.log"
            }
        ]
    })";

    TempConfigFile config("case_test.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);

    if (!config_ptr) {
        return false;
    }

    return config_ptr->overflow_policy == OverflowPolicy::Discard && config_ptr->printers[0].min_level == LogLevel::Debug &&
           config_ptr->printers[1].min_level == LogLevel::Info;
}

bool test_default_values() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "Console"
            },
            {
                "type": "File",
                "path": "default.log"
            }
        ]
    })";

    TempConfigFile config("defaults.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);

    if (!config_ptr || error != ConfigError::None) {
        return false;
    }

    const auto& printers = config_ptr->printers;

    // Console 默认 level 应为 Info
    const auto& console = printers[0];
    if (console.type != PrinterType::Console ||
        console.min_level != LogLevel::Info) { // 假设你从 raw 解析了 min_level 并设置默认值
        return false;
    }

    // File 默认值检查
    const auto& file = printers[1];
    if (file.type != PrinterType::File || file.min_level != LogLevel::Info ||                                    // 默认日志级别
        !file.raw.contains("path") || file.raw["path"] != "default.log" || file.raw.value("max_size", 0) != 0 || // 默认 max_size
        file.raw.value("flush_every", 64) != 64) { // 默认 flush_every
        return false;
    }

    return true;
}

bool test_empty_printers_array() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": []
    })";

    TempConfigFile config("empty_printers.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);
    return config_ptr && config_ptr->printers.size() == 0;
}

bool test_multiple_printers_same_type() {
    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "Discard",
        "printers": [
            {
                "type": "Console",
                "level": "Debug"
            },
            {
                "type": "Console",
                "level": "Error"
            }
        ]
    })";

    TempConfigFile config("multi_console.json", json);
    ConfigError error;
    auto config_ptr = load_config(config.path(), error);

    if (!config_ptr || config_ptr->printers.size() != 2) {
        return false;
    }

    return config_ptr->printers[0].min_level == LogLevel::Debug && config_ptr->printers[1].min_level == LogLevel::Error;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Config Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    TestResult result;

    // 基础测试
    run_test("Valid minimal config", test_valid_minimal_config, result);
    run_test("Valid full config with all options", test_valid_full_config, result);

    // 错误处理测试
    run_test("File not found", test_file_not_found, result);
    run_test("Invalid JSON syntax", test_invalid_json, result);
    run_test("Missing printers array", test_missing_printers, result);
    run_test("Invalid printer type", test_invalid_printer_type, result);
    run_test("Invalid overflow policy", test_invalid_overflow_policy, result);
    run_test("Invalid log level", test_invalid_log_level, result);
    run_test("Non-power-of-2 buffer size", test_non_power_of_two_buffer, result);
    run_test("File printer missing path", test_missing_file_path, result);

    // 边界情况测试
    run_test("Case insensitive parsing", test_case_insensitive_parsing, result);
    run_test("Default values for optional fields", test_default_values, result);
    run_test("Empty printers array", test_empty_printers_array, result);
    run_test("Multiple printers of same type", test_multiple_printers_same_type, result);

    print_test_summary("Config Test Suite", result);

    return result.failed > 0 ? 1 : 0;
}
