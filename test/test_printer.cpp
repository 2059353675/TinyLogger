#include "test_common.h"
#include <TinyLogger/printer_console.h>
#include <TinyLogger/printer_file.h>
#include <TinyLogger/types.h>

using namespace TinyLogger;
using namespace TinyLogger::test;

// ==================== ConsolePrinter 测试 ====================

bool test_console_printer_init() {
    try {
        PrinterConfig config;
        config.type = PrinterType::Console;
        config.min_level = LogLevel::Debug;
        ConsolePrinter printer(config);
        return true;
    } catch (...) { return false; }
}

bool test_console_printer_write() {
    PrinterConfig config;
    config.type = PrinterType::Console;
    config.min_level = LogLevel::Debug;
    ConsolePrinter printer(config);

    LogEvent event = create_test_event(LogLevel::Info, "Test message");

    try {
        printer.write(event);
        printer.flush();
        return true;
    } catch (...) { return false; }
}

bool test_console_printer_write_multiline() {
    PrinterConfig config;
    config.type = PrinterType::Console;
    config.min_level = LogLevel::Debug;
    ConsolePrinter printer(config);

    LogEvent event = create_test_event(LogLevel::Debug, "Line 1\nLine 2\nLine 3");

    try {
        printer.write(event);
        printer.flush();
        return true;
    } catch (...) { return false; }
}

// ==================== FilePrinter 测试 ====================

bool test_file_printer_init() {
    TempLogFile test_file("init.log");

    try {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = test_file.path();
        FilePrinter printer(config);
        return true;
    } catch (...) { return false; }
}

bool test_file_printer_write() {
    TempLogFile test_file("write.log");

    {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = test_file.path();
        FilePrinter printer(config);
        printer.set_flush_every(1); // 立即 flush

        LogEvent event = create_test_event(LogLevel::Info, "Hello, File!");
        printer.write(event);
        printer.flush();
    }

    std::string content = test_file.read_content();
    return content.find("Hello, File!") != std::string::npos;
}

bool test_file_printer_multiple_writes() {
    TempLogFile test_file("multi_write.log");

    {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = test_file.path();
        FilePrinter printer(config);
        printer.set_flush_every(1);

        for (int i = 0; i < 10; ++i) {
            char msg[64];
            std::snprintf(msg, sizeof(msg), "Message %d", i);
            LogEvent event = create_test_event(LogLevel::Info, msg);
            printer.write(event);
        }
        printer.flush();
    }

    std::string content = test_file.read_content();

    // 检查所有消息都在文件中
    for (int i = 0; i < 10; ++i) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "Message %d", i);
        if (content.find(msg) == std::string::npos) {
            return false;
        }
    }

    return true;
}

bool test_file_printer_flush() {
    TempLogFile test_file("flush.log");

    {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = test_file.path();
        FilePrinter printer(config);
        printer.set_flush_every(1000); // 很大的值，不会自动 flush

        LogEvent event = create_test_event(LogLevel::Info, "Flush test");
        printer.write(event);

        // 手动 flush
        printer.flush();
    }

    std::string content = test_file.read_content();
    return content.find("Flush test") != std::string::npos;
}

bool test_file_printer_rotation() {
    TempLogFile test_file("rotate.log");
    std::string backup_file = test_file.path() + ".1";
    std::remove(backup_file.c_str());

    {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = test_file.path();
        FilePrinter printer(config);
        printer.set_flush_every(1);
        printer.set_max_size(100); // 很小的值触发滚动

        // 写入足够多的数据触发滚动
        for (int i = 0; i < 20; ++i) {
            LogEvent event = create_test_event(LogLevel::Info, "Rotation test message ");
            printer.write(event);
        }
        printer.flush();
    }

    // 检查备份文件是否存在
    std::ifstream backup(backup_file);
    bool backup_exists = backup.is_open();
    if (backup_exists) {
        backup.close();
    }

    return backup_exists;
}

bool test_file_printer_append_mode() {
    TempLogFile test_file("append.log");

    // 第一次写入
    {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = test_file.path();
        FilePrinter printer(config);
        printer.set_flush_every(1);

        LogEvent event = create_test_event(LogLevel::Info, "First");
        printer.write(event);
        printer.flush();
    }

    // 第二次写入
    {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = test_file.path();
        FilePrinter printer(config);
        printer.set_flush_every(1);

        LogEvent event = create_test_event(LogLevel::Info, "Second");
        printer.write(event);
        printer.flush();
    }

    std::string content = test_file.read_content();
    return content.find("First") != std::string::npos && content.find("Second") != std::string::npos;
}

// ==================== Printer 级别过滤测试 ====================

bool test_printer_level_filtering() {
    PrinterConfig config;
    config.type = PrinterType::Console;
    config.min_level = LogLevel::Error; // 只记录 Error 及以上

    ConsolePrinter printer(config);

    LogEvent debug_event = create_test_event(LogLevel::Debug, "Debug msg");
    LogEvent info_event = create_test_event(LogLevel::Info, "Info msg");
    LogEvent error_event = create_test_event(LogLevel::Error, "Error msg");
    LogEvent fatal_event = create_test_event(LogLevel::Fatal, "Fatal msg");

    if (printer.should_log(debug_event.level)) {
        return false;
    }

    if (printer.should_log(info_event.level)) {
        return false;
    }

    if (!printer.should_log(error_event.level)) {
        return false;
    }

    if (!printer.should_log(fatal_event.level)) {
        return false;
    }

    return true;
}

bool test_printer_level_ordering() {
    // 验证日志级别顺序：Debug < Info < Error < Fatal
    LogLevel levels[] = {LogLevel::Debug, LogLevel::Info, LogLevel::Error, LogLevel::Fatal};

    for (size_t i = 0; i < 4; ++i) {
        PrinterConfig config;
        config.type = PrinterType::Console;
        config.min_level = levels[i];
        ConsolePrinter printer(config);

        // 应该记录当前级别及更高级别
        for (size_t j = i; j < 4; ++j) {
            if (!printer.should_log(levels[j])) {
                return false;
            }
        }

        // 不应该记录更低级别
        for (size_t j = 0; j < i; ++j) {
            if (printer.should_log(levels[j])) {
                return false;
            }
        }
    }

    return true;
}

// ==================== 异常处理测试 ====================

bool test_file_printer_invalid_path() {
    // 使用无效路径
    std::string invalid_path = "/invalid/path/that/does/not/exist/test.log";

    try {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = invalid_path;
        FilePrinter printer(config);

        // 应该 fallback 到 stderr，不崩溃
        LogEvent event = create_test_event(LogLevel::Error, "Test");
        printer.write(event);
        printer.flush();

        return true;
    } catch (...) { return false; }
}

bool test_file_printer_empty_message() {
    TempLogFile test_file("empty.log");

    {
        PrinterConfig config;
        config.type = PrinterType::File;
        config.min_level = LogLevel::Debug;
        config.raw["path"] = test_file.path();
        FilePrinter printer(config);
        printer.set_flush_every(1);

        LogEvent event = create_test_event(LogLevel::Info, "");
        printer.write(event);
        printer.flush();
    }

    // 不应该崩溃
    return true;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Printer Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    TestResult result;

    // ConsolePrinter 测试
    run_test("ConsolePrinter init", test_console_printer_init, result);
    run_test("ConsolePrinter write", test_console_printer_write, result);
    run_test("ConsolePrinter write multiline", test_console_printer_write_multiline, result);

    // FilePrinter 测试
    run_test("FilePrinter init", test_file_printer_init, result);
    run_test("FilePrinter write", test_file_printer_write, result);
    run_test("FilePrinter multiple writes", test_file_printer_multiple_writes, result);
    run_test("FilePrinter flush", test_file_printer_flush, result);
    run_test("FilePrinter rotation", test_file_printer_rotation, result);
    run_test("FilePrinter append mode", test_file_printer_append_mode, result);

    // 级别过滤测试
    run_test("Printer level filtering", test_printer_level_filtering, result);
    run_test("Printer level ordering", test_printer_level_ordering, result);

    // 异常处理测试
    run_test("FilePrinter invalid path", test_file_printer_invalid_path, result);
    run_test("FilePrinter empty message", test_file_printer_empty_message, result);

    print_test_summary("Printer Test Suite", result);

    return result.failed > 0 ? 1 : 0;
}
