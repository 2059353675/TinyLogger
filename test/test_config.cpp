#include <TinyLogger/config.h>
#include <fstream>
#include <iostream>
#include <string>

using namespace TinyLogger;

// ==================== 工具函数 ====================

static std::string create_temp_config(const std::string& content, const std::string& filename) {
    std::string path = "test_config_" + filename;
    std::ofstream ofs(path);
    ofs << content;
    ofs.close();
    return path;
}

static void cleanup_config(const std::string& path) {
    std::remove(path.c_str());
}

// ==================== 基础配置测试 ====================

bool test_valid_minimal_config() {
    std::cout << "[TEST] Valid minimal config... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "discard",
        "printers": [
            {
                "type": "console",
                "level": "info"
            }
        ]
    })";

    std::string path = create_temp_config(json, "minimal.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (!config || error != ConfigError::None) {
        std::cout << "FAILED (load error: " << static_cast<int>(error) << ")" << std::endl;
        return false;
    }

    if (config->buffer_size != 256 || config->overflow_policy != OverflowPolicy::Discard || config->printers.size() != 1) {
        std::cout << "FAILED (value mismatch)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_valid_full_config() {
    std::cout << "[TEST] Valid full config with all options... ";

    std::string json = R"({
        "buffer_size": 512,
        "overflow_policy": "block",
        "printers": [
            {
                "type": "console",
                "level": "debug"
            },
            {
                "type": "file",
                "level": "info",
                "path": "test.log",
                "max_size": 1048576,
                "flush_every": 128
            }
        ]
    })";

    std::string path = create_temp_config(json, "full.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (!config) {
        std::cout << "FAILED (load error)" << std::endl;
        return false;
    }

    if (config->buffer_size != 512 || config->overflow_policy != OverflowPolicy::Block || config->printers.size() != 2) {
        std::cout << "FAILED (basic values)" << std::endl;
        return false;
    }

    // 检查 console printer
    if (config->printers[0].type != PrinterType::Console || config->printers[0].min_level != LogLevel::Debug) {
        std::cout << "FAILED (console printer)" << std::endl;
        return false;
    }

    // 检查 file printer
    if (config->printers[1].type != PrinterType::File || config->printers[1].min_level != LogLevel::Info ||
        config->printers[1].file_path != "test.log" || config->printers[1].max_size != 1048576 ||
        config->printers[1].flush_every != 128) {
        std::cout << "FAILED (file printer)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 错误处理测试 ====================

bool test_file_not_found() {
    std::cout << "[TEST] File not found... ";

    ConfigError error;
    auto config = load_config("nonexistent_file.json", error);

    if (config || error != ConfigError::FileNotFound) {
        std::cout << "FAILED (expected FileNotFound, got " << static_cast<int>(error) << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_invalid_json() {
    std::cout << "[TEST] Invalid JSON syntax... ";

    std::string json = "{ invalid json }";
    std::string path = create_temp_config(json, "invalid.json");

    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (config || error != ConfigError::ParseError) {
        std::cout << "FAILED (expected ParseError, got " << static_cast<int>(error) << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_missing_printers() {
    std::cout << "[TEST] Missing printers array... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "discard"
    })";

    std::string path = create_temp_config(json, "no_printers.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (config || error != ConfigError::InvalidPrinterType) {
        std::cout << "FAILED (expected InvalidPrinterType, got " << static_cast<int>(error) << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_invalid_printer_type() {
    std::cout << "[TEST] Invalid printer type... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "discard",
        "printers": [
            {
                "type": "invalid_type",
                "level": "info"
            }
        ]
    })";

    std::string path = create_temp_config(json, "invalid_type.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (config || error != ConfigError::InvalidPrinterType) {
        std::cout << "FAILED (expected InvalidPrinterType, got " << static_cast<int>(error) << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_invalid_overflow_policy() {
    std::cout << "[TEST] Invalid overflow policy... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "invalid_policy",
        "printers": [
            {
                "type": "console",
                "level": "info"
            }
        ]
    })";

    std::string path = create_temp_config(json, "invalid_overflow.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (config || error != ConfigError::InvalidOverflowPolicy) {
        std::cout << "FAILED (expected InvalidOverflowPolicy, got " << static_cast<int>(error) << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_invalid_log_level() {
    std::cout << "[TEST] Invalid log level... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "discard",
        "printers": [
            {
                "type": "console",
                "level": "verbose"
            }
        ]
    })";

    std::string path = create_temp_config(json, "invalid_level.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (config || error != ConfigError::InvalidLevel) {
        std::cout << "FAILED (expected InvalidLevel, got " << static_cast<int>(error) << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_non_power_of_two_buffer() {
    std::cout << "[TEST] Non-power-of-2 buffer size... ";

    std::string json = R"({
        "buffer_size": 100,
        "overflow_policy": "discard",
        "printers": [
            {
                "type": "console",
                "level": "info"
            }
        ]
    })";

    std::string path = create_temp_config(json, "bad_buffer.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (config) {
        std::cout << "FAILED (should reject non-power-of-2)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_missing_file_path() {
    std::cout << "[TEST] File printer missing path... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "discard",
        "printers": [
            {
                "type": "file",
                "level": "info"
            }
        ]
    })";

    std::string path = create_temp_config(json, "missing_path.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (config || error != ConfigError::InvalidPrinterType) {
        std::cout << "FAILED (expected InvalidPrinterType, got " << static_cast<int>(error) << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 边界情况测试 ====================

bool test_case_insensitive_parsing() {
    std::cout << "[TEST] Case insensitive parsing... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "DISCARD",
        "printers": [
            {
                "type": "Console",
                "level": "DEBUG"
            },
            {
                "type": "FILE",
                "level": "Info",
                "path": "test.log"
            }
        ]
    })";

    std::string path = create_temp_config(json, "case_test.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (!config) {
        std::cout << "FAILED (load error)" << std::endl;
        return false;
    }

    if (config->overflow_policy != OverflowPolicy::Discard || config->printers[0].min_level != LogLevel::Debug ||
        config->printers[1].min_level != LogLevel::Info) {
        std::cout << "FAILED (case sensitivity)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_default_values() {
    std::cout << "[TEST] Default values for optional fields... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "discard",
        "printers": [
            {
                "type": "console"
            }
        ]
    })";

    std::string path = create_temp_config(json, "defaults.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (!config) {
        std::cout << "FAILED (load error)" << std::endl;
        return false;
    }

    // 检查默认值
    if (config->printers[0].min_level != LogLevel::Info) {
        std::cout << "FAILED (default level)" << std::endl;
        return false;
    }

    if (config->printers[0].max_size != 0 || config->printers[0].flush_every != 64) {
        std::cout << "FAILED (default file settings)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_empty_printers_array() {
    std::cout << "[TEST] Empty printers array... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "discard",
        "printers": []
    })";

    std::string path = create_temp_config(json, "empty_printers.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (!config || config->printers.size() != 0) {
        std::cout << "FAILED" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_multiple_printers_same_type() {
    std::cout << "[TEST] Multiple printers of same type... ";

    std::string json = R"({
        "buffer_size": 256,
        "overflow_policy": "discard",
        "printers": [
            {
                "type": "console",
                "level": "debug"
            },
            {
                "type": "console",
                "level": "error"
            }
        ]
    })";

    std::string path = create_temp_config(json, "multi_console.json");
    ConfigError error;
    auto config = load_config(path, error);
    cleanup_config(path);

    if (!config || config->printers.size() != 2) {
        std::cout << "FAILED (count)" << std::endl;
        return false;
    }

    if (config->printers[0].min_level != LogLevel::Debug || config->printers[1].min_level != LogLevel::Error) {
        std::cout << "FAILED (levels)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Config Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0;
    int failed = 0;

    auto run_test = [&](bool (*test_func)()) {
        if (test_func()) {
            passed++;
        } else {
            failed++;
        }
    };

    // 基础测试
    run_test(test_valid_minimal_config);
    run_test(test_valid_full_config);

    // 错误处理测试
    run_test(test_file_not_found);
    run_test(test_invalid_json);
    run_test(test_missing_printers);
    run_test(test_invalid_printer_type);
    run_test(test_invalid_overflow_policy);
    run_test(test_invalid_log_level);
    run_test(test_non_power_of_two_buffer);
    run_test(test_missing_file_path);

    // 边界情况测试
    run_test(test_case_insensitive_parsing);
    run_test(test_default_values);
    run_test(test_empty_printers_array);
    run_test(test_multiple_printers_same_type);

    std::cout << "========================================" << std::endl;
    std::cout << "  Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed > 0 ? 1 : 0;
}
