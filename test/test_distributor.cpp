#include <TinyLogger/distributor.h>
#include <TinyLogger/printer.h>
#include <TinyLogger/ring_buffer.h>
#include <TinyLogger/types.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace TinyLogger;

// ==================== 测试用 Mock Printer ====================

class MockPrinter : public Printer
{
public:
    MockPrinter() : write_count_(0) {
    }

    void init(const json& cfg) override {
        initialized_ = true;
    }

    void write(const LogEvent& event) override {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(event);
        write_count_.fetch_add(1);
    }

    void flush() override {
        flushed_ = true;
    }

    size_t get_write_count() const {
        return write_count_.load();
    }

    const std::vector<LogEvent>& get_events() const {
        return events_;
    }

    bool is_initialized() const {
        return initialized_;
    }

    bool is_flushed() const {
        return flushed_;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.clear();
        write_count_.store(0);
        flushed_ = false;
    }

private:
    std::mutex mutex_;
    std::vector<LogEvent> events_;
    std::atomic<size_t> write_count_;
    bool initialized_ = false;
    bool flushed_ = false;
};

// ==================== 工具函数 ====================

static LogEvent create_test_event(LogLevel level, const char* msg) {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

    size_t len = std::strlen(msg);
    return LogEvent(level, ts, msg, len);
}

// ==================== Distributor 基础测试 ====================

bool test_distributor_creation() {
    std::cout << "[TEST] Distributor creation... ";

    try {
        RingBuffer rb(256);
        Distributor distributor(rb);
        std::cout << "PASSED" << std::endl;
        return true;
    } catch (...) {
        std::cout << "FAILED" << std::endl;
        return false;
    }
}

bool test_distributor_start_stop() {
    std::cout << "[TEST] Distributor start/stop... ";

    RingBuffer rb(256);
    Distributor distributor(rb);

    distributor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distributor.stop();

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_distributor_add_printer() {
    std::cout << "[TEST] Distributor add printer... ";

    RingBuffer rb(256);
    Distributor distributor(rb);

    auto printer = std::make_unique<MockPrinter>();
    distributor.add_printer(std::move(printer));

    distributor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distributor.stop();

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Distributor 分发测试 ====================

bool test_distributor_single_event() {
    std::cout << "[TEST] Distributor single event... ";

    RingBuffer rb(256);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    LogEvent event = create_test_event(LogLevel::Info, "Test event");
    rb.enqueue(std::move(event));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    if (printer_ptr->get_write_count() != 1) {
        std::cout << "FAILED (expected 1, got " << printer_ptr->get_write_count() << ")" << std::endl;
        return false;
    }

    if (printer_ptr->get_events()[0].level != LogLevel::Info) {
        std::cout << "FAILED (level mismatch)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_distributor_multiple_events() {
    std::cout << "[TEST] Distributor multiple events... ";

    RingBuffer rb(256);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    const int EVENT_COUNT = 100;
    for (int i = 0; i < EVENT_COUNT; ++i) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "Event %d", i);
        LogEvent event = create_test_event(LogLevel::Info, msg);
        rb.enqueue(std::move(event));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    distributor->stop();

    if (printer_ptr->get_write_count() != EVENT_COUNT) {
        std::cout << "FAILED (expected " << EVENT_COUNT << ", got " << printer_ptr->get_write_count() << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_distributor_multiple_printers() {
    std::cout << "[TEST] Distributor multiple printers... ";

    RingBuffer rb(256);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer1 = std::make_unique<MockPrinter>();
    printer1->set_level(LogLevel::Debug);
    MockPrinter* printer1_ptr = printer1.get();
    distributor->add_printer(std::move(printer1));

    auto printer2 = std::make_unique<MockPrinter>();
    printer2->set_level(LogLevel::Info);
    MockPrinter* printer2_ptr = printer2.get();
    distributor->add_printer(std::move(printer2));

    distributor->start();

    LogEvent event = create_test_event(LogLevel::Error, "Multi-printer test");
    rb.enqueue(std::move(event));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    // 两个 printer 都应该收到事件
    if (printer1_ptr->get_write_count() != 1 || printer2_ptr->get_write_count() != 1) {
        std::cout << "FAILED (p1: " << printer1_ptr->get_write_count() << ", p2: " << printer2_ptr->get_write_count() << ")"
                  << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Distributor 级别过滤测试 ====================

bool test_distributor_level_filtering() {
    std::cout << "[TEST] Distributor level filtering... ";

    RingBuffer rb(256);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Error); // 只记录 Error 及以上
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    // 写入不同级别的日志
    LogEvent debug_event = create_test_event(LogLevel::Debug, "Debug");
    LogEvent info_event = create_test_event(LogLevel::Info, "Info");
    LogEvent error_event = create_test_event(LogLevel::Error, "Error");
    LogEvent fatal_event = create_test_event(LogLevel::Fatal, "Fatal");

    rb.enqueue(std::move(debug_event));
    rb.enqueue(std::move(info_event));
    rb.enqueue(std::move(error_event));
    rb.enqueue(std::move(fatal_event));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    // 应该只收到 Error 和 Fatal
    if (printer_ptr->get_write_count() != 2) {
        std::cout << "FAILED (expected 2, got " << printer_ptr->get_write_count() << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Distributor 并发测试 ====================

bool test_distributor_concurrent_enqueue() {
    std::cout << "[TEST] Distributor concurrent enqueue... ";

    RingBuffer rb(1024);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    constexpr int THREAD_COUNT = 4;
    constexpr int EVENTS_PER_THREAD = 250;

    std::vector<std::thread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&rb, t]() {
            for (int i = 0; i < EVENTS_PER_THREAD; ++i) {
                char msg[64];
                std::snprintf(msg, sizeof(msg), "T%d-E%d", t, i);
                LogEvent event = create_test_event(LogLevel::Info, msg);

                // 如果缓冲区满了，重试
                while (!rb.enqueue(std::move(event))) {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    distributor->stop();

    int expected = THREAD_COUNT * EVENTS_PER_THREAD;
    if (printer_ptr->get_write_count() != expected) {
        std::cout << "FAILED (expected " << expected << ", got " << printer_ptr->get_write_count() << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Distributor 生命周期测试 ====================

bool test_distributor_drain_on_stop() {
    std::cout << "[TEST] Distributor drain on stop... ";

    RingBuffer rb(256);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    // 写入一些事件
    for (int i = 0; i < 50; ++i) {
        LogEvent event = create_test_event(LogLevel::Info, "Drain test");
        rb.enqueue(std::move(event));
    }

    // 立即停止，应该清空剩余事件
    distributor->stop();

    // 允许一些事件丢失（还未 dequeue 的）
    // 但不应该全部丢失
    if (printer_ptr->get_write_count() == 0) {
        std::cout << "FAILED (no events drained)" << std::endl;
        return false;
    }

    std::cout << "PASSED (drained " << printer_ptr->get_write_count() << " events)" << std::endl;
    return true;
}

bool test_distributor_flush_on_stop() {
    std::cout << "[TEST] Distributor flush on stop... ";

    RingBuffer rb(256);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    LogEvent event = create_test_event(LogLevel::Info, "Flush test");
    rb.enqueue(std::move(event));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distributor->stop();

    // stop 应该调用 flush
    if (!printer_ptr->is_flushed()) {
        std::cout << "FAILED (not flushed)" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_distributor_double_start_stop() {
    std::cout << "[TEST] Distributor double start/stop... ";

    RingBuffer rb(256);
    Distributor distributor(rb);

    // 两次 start 应该不会崩溃
    distributor.start();
    distributor.start(); // 应该被忽略

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 两次 stop 应该不会崩溃
    distributor.stop();
    distributor.stop(); // 应该被忽略

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== Distributor 批量处理测试 ====================

bool test_distributor_batch_processing() {
    std::cout << "[TEST] Distributor batch processing... ";

    RingBuffer rb(1024);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    // 快速写入大量事件，测试批处理
    const int EVENT_COUNT = 200;
    for (int i = 0; i < EVENT_COUNT; ++i) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "Batch %d", i);
        LogEvent event = create_test_event(LogLevel::Info, msg);
        rb.enqueue(std::move(event));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    distributor->stop();

    if (printer_ptr->get_write_count() != EVENT_COUNT) {
        std::cout << "FAILED (expected " << EVENT_COUNT << ", got " << printer_ptr->get_write_count() << ")" << std::endl;
        return false;
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Distributor Test Suite" << std::endl;
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
    run_test(test_distributor_creation);
    run_test(test_distributor_start_stop);
    run_test(test_distributor_add_printer);

    // 分发测试
    run_test(test_distributor_single_event);
    run_test(test_distributor_multiple_events);
    run_test(test_distributor_multiple_printers);

    // 级别过滤测试
    run_test(test_distributor_level_filtering);

    // 并发测试
    run_test(test_distributor_concurrent_enqueue);

    // 生命周期测试
    run_test(test_distributor_drain_on_stop);
    run_test(test_distributor_flush_on_stop);
    run_test(test_distributor_double_start_stop);

    // 批量处理测试
    run_test(test_distributor_batch_processing);

    std::cout << "========================================" << std::endl;
    std::cout << "  Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed > 0 ? 1 : 0;
}
