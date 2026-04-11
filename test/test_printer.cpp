#include <TinyLogger/printer_console.h>
#include <TinyLogger/printer_file.h>
#include <TinyLogger/types.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

using namespace TinyLogger;

// ==================== 工具函数 ====================

static LogEvent create_test_event(LogLevel level, const char* msg) {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    
    size_t len = std::strlen(msg);
    return LogEvent(level, ts, msg, len);
}

static std::string read_file_content(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
}

// ==================== ConsolePrinter 测试 ====================

bool test_console_printer_init() {
    std::cout << "[TEST] ConsolePrinter init... ";
    
    try {
        ConsolePrinter printer;
        json cfg;
        printer.init(cfg);
        std::cout << "PASSED" << std::endl;
        return true;
    } catch (...) {
        std::cout << "FAILED" << std::endl;
        return false;
    }
}

bool test_console_printer_write() {
    std::cout << "[TEST] ConsolePrinter write... ";
    
    ConsolePrinter printer;
    json cfg;
    printer.init(cfg);
    printer.set_level(LogLevel::Debug);
    
    LogEvent event = create_test_event(LogLevel::Info, "Test message");
    
    // 注意：这会输出到 stdout，难以自动化验证
    // 这里主要测试不崩溃
    try {
        printer.write(event);
        printer.flush();
        std::cout << "PASSED" << std::endl;
        return true;
    } catch (...) {
        std::cout << "FAILED" << std::endl;
        return false;
    }
}

bool test_console_printer_write_multiline() {
    std::cout << "[TEST] ConsolePrinter write multiline... ";
    
    ConsolePrinter printer;
    json cfg;
    printer.init(cfg);
    printer.set_level(LogLevel::Debug);
    
    LogEvent event = create_test_event(LogLevel::Debug, "Line 1\nLine 2\nLine 3");
    
    try {
        printer.write(event);
        printer.flush();
        std::cout << "PASSED" << std::endl;
        return true;
    } catch (...) {
        std::cout << "FAILED" << std::endl;
        return false;
    }
}

// ==================== FilePrinter 测试 ====================

bool test_file_printer_init() {
    std::cout << "[TEST] FilePrinter init... ";
    
    std::string test_file = "test_printer_file.log";
    
    try {
        FilePrinter printer(test_file);
        json cfg;
        printer.init(cfg);
        
        // 检查文件是否创建
        std::ifstream ifs(test_file);
        if (!ifs.is_open()) {
            std::cout << "FAILED (file not created)" << std::endl;
            return false;
        }
        
        std::remove(test_file.c_str());
        std::cout << "PASSED" << std::endl;
        return true;
    } catch (...) {
        std::remove(test_file.c_str());
        std::cout << "FAILED" << std::endl;
        return false;
    }
}

bool test_file_printer_write() {
    std::cout << "[TEST] FilePrinter write... ";
    
    std::string test_file = "test_printer_write.log";
    std::remove(test_file.c_str());
    
    {
        FilePrinter printer(test_file);
        json cfg;
        printer.init(cfg);
        printer.set_level(LogLevel::Debug);
        printer.set_flush_every(1); // 立即 flush
        
        LogEvent event = create_test_event(LogLevel::Info, "Hello, File!");
        printer.write(event);
        printer.flush();
    } // 确保析构
    
    std::string content = read_file_content(test_file);
    std::remove(test_file.c_str());
    
    if (content.find("Hello, File!") == std::string::npos) {
        std::cout << "FAILED (content mismatch: " << content << ")" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_file_printer_multiple_writes() {
    std::cout << "[TEST] FilePrinter multiple writes... ";
    
    std::string test_file = "test_multi_write.log";
    std::remove(test_file.c_str());
    
    {
        FilePrinter printer(test_file);
        json cfg;
        printer.init(cfg);
        printer.set_level(LogLevel::Debug);
        printer.set_flush_every(1);
        
        for (int i = 0; i < 10; ++i) {
            char msg[64];
            std::snprintf(msg, sizeof(msg), "Message %d", i);
            LogEvent event = create_test_event(LogLevel::Info, msg);
            printer.write(event);
        }
        printer.flush();
    }
    
    std::string content = read_file_content(test_file);
    std::remove(test_file.c_str());
    
    // 检查所有消息都在文件中
    for (int i = 0; i < 10; ++i) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "Message %d", i);
        if (content.find(msg) == std::string::npos) {
            std::cout << "FAILED (missing: " << msg << ")" << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_file_printer_flush() {
    std::cout << "[TEST] FilePrinter flush... ";
    
    std::string test_file = "test_flush.log";
    std::remove(test_file.c_str());
    
    {
        FilePrinter printer(test_file);
        json cfg;
        printer.init(cfg);
        printer.set_level(LogLevel::Debug);
        printer.set_flush_every(1000); // 很大的值，不会自动 flush
        
        LogEvent event = create_test_event(LogLevel::Info, "Flush test");
        printer.write(event);
        
        // 手动 flush
        printer.flush();
    }
    
    std::string content = read_file_content(test_file);
    std::remove(test_file.c_str());
    
    if (content.find("Flush test") == std::string::npos) {
        std::cout << "FAILED" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_file_printer_rotation() {
    std::cout << "[TEST] FilePrinter rotation... ";
    
    std::string test_file = "test_rotate.log";
    std::string backup_file = test_file + ".1";
    std::remove(test_file.c_str());
    std::remove(backup_file.c_str());
    
    {
        FilePrinter printer(test_file);
        json cfg;
        printer.init(cfg);
        printer.set_level(LogLevel::Debug);
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
    
    std::remove(test_file.c_str());
    std::remove(backup_file.c_str());
    
    if (!backup_exists) {
        std::cout << "FAILED (backup not created)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_file_printer_append_mode() {
    std::cout << "[TEST] FilePrinter append mode... ";
    
    std::string test_file = "test_append.log";
    std::remove(test_file.c_str());
    
    // 第一次写入
    {
        FilePrinter printer(test_file);
        json cfg;
        printer.init(cfg);
        printer.set_level(LogLevel::Debug);
        printer.set_flush_every(1);
        
        LogEvent event = create_test_event(LogLevel::Info, "First");
        printer.write(event);
        printer.flush();
    }
    
    // 第二次写入
    {
        FilePrinter printer(test_file);
        json cfg;
        printer.init(cfg);
        printer.set_level(LogLevel::Debug);
        printer.set_flush_every(1);
        
        LogEvent event = create_test_event(LogLevel::Info, "Second");
        printer.write(event);
        printer.flush();
    }
    
    std::string content = read_file_content(test_file);
    std::remove(test_file.c_str());
    
    if (content.find("First") == std::string::npos || 
        content.find("Second") == std::string::npos) {
        std::cout << "FAILED (append failed: " << content << ")" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Printer 级别过滤测试 ====================

bool test_printer_level_filtering() {
    std::cout << "[TEST] Printer level filtering... ";
    
    ConsolePrinter printer;
    json cfg;
    printer.init(cfg);
    printer.set_level(LogLevel::Error); // 只记录 Error 及以上
    
    LogEvent debug_event = create_test_event(LogLevel::Debug, "Debug msg");
    LogEvent info_event = create_test_event(LogLevel::Info, "Info msg");
    LogEvent error_event = create_test_event(LogLevel::Error, "Error msg");
    LogEvent fatal_event = create_test_event(LogLevel::Fatal, "Fatal msg");
    
    if (printer.should_log(debug_event.level)) {
        std::cout << "FAILED (should filter Debug)" << std::endl;
        return false;
    }
    
    if (printer.should_log(info_event.level)) {
        std::cout << "FAILED (should filter Info)" << std::endl;
        return false;
    }
    
    if (!printer.should_log(error_event.level)) {
        std::cout << "FAILED (should log Error)" << std::endl;
        return false;
    }
    
    if (!printer.should_log(fatal_event.level)) {
        std::cout << "FAILED (should log Fatal)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_printer_level_ordering() {
    std::cout << "[TEST] Printer level ordering... ";
    
    // 验证日志级别顺序：Debug < Info < Error < Fatal
    LogLevel levels[] = {LogLevel::Debug, LogLevel::Info, LogLevel::Error, LogLevel::Fatal};
    
    for (size_t i = 0; i < 4; ++i) {
        ConsolePrinter printer;
        json cfg;
        printer.init(cfg);
        printer.set_level(levels[i]);
        
        // 应该记录当前级别及更高级别
        for (size_t j = i; j < 4; ++j) {
            if (!printer.should_log(levels[j])) {
                std::cout << "FAILED (level " << j << " should be logged when min is " << i << ")" << std::endl;
                return false;
            }
        }
        
        // 不应该记录更低级别
        for (size_t j = 0; j < i; ++j) {
            if (printer.should_log(levels[j])) {
                std::cout << "FAILED (level " << j << " should be filtered when min is " << i << ")" << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 异常处理测试 ====================

bool test_file_printer_invalid_path() {
    std::cout << "[TEST] FilePrinter invalid path... ";
    
    // 使用无效路径
    std::string invalid_path = "/invalid/path/that/does/not/exist/test.log";
    
    try {
        FilePrinter printer(invalid_path);
        json cfg;
        printer.init(cfg);
        
        // 应该 fallback 到 stderr，不崩溃
        LogEvent event = create_test_event(LogLevel::Error, "Test");
        printer.write(event);
        printer.flush();
        
        std::cout << "PASSED" << std::endl;
        return true;
    } catch (...) {
        std::cout << "FAILED (exception)" << std::endl;
        return false;
    }
}

bool test_file_printer_empty_message() {
    std::cout << "[TEST] FilePrinter empty message... ";
    
    std::string test_file = "test_empty.log";
    std::remove(test_file.c_str());
    
    {
        FilePrinter printer(test_file);
        json cfg;
        printer.init(cfg);
        printer.set_level(LogLevel::Debug);
        printer.set_flush_every(1);
        
        LogEvent event = create_test_event(LogLevel::Info, "");
        printer.write(event);
        printer.flush();
    }
    
    // 不应该崩溃
    std::remove(test_file.c_str());
    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Printer Test Suite" << std::endl;
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
    
    // ConsolePrinter 测试
    run_test(test_console_printer_init);
    run_test(test_console_printer_write);
    run_test(test_console_printer_write_multiline);
    
    // FilePrinter 测试
    run_test(test_file_printer_init);
    run_test(test_file_printer_write);
    run_test(test_file_printer_multiple_writes);
    run_test(test_file_printer_flush);
    run_test(test_file_printer_rotation);
    run_test(test_file_printer_append_mode);
    
    // 级别过滤测试
    run_test(test_printer_level_filtering);
    run_test(test_printer_level_ordering);
    
    // 异常处理测试
    run_test(test_file_printer_invalid_path);
    run_test(test_file_printer_empty_message);
    
    std::cout << "========================================" << std::endl;
    std::cout << "  Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
