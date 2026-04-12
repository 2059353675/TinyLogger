#include <TinyLogger/distributor.h>
#include <TinyLogger/printer.h>
#include <TinyLogger/ring_buffer.h>
#include <TinyLogger/types.h>
#include "test_common.h"

using namespace TinyLogger;
using namespace TinyLogger::test;

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

// ==================== Distributor 基础测试 ====================

bool test_distributor_creation() {
    try {
        RingBuffer rb(256);
        Distributor distributor(rb);
        return true;
    } catch (...) {
        return false;
    }
}

bool test_distributor_start_stop() {
    RingBuffer rb(256);
    Distributor distributor(rb);

    distributor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distributor.stop();

    return true;
}

bool test_distributor_add_printer() {
    RingBuffer rb(256);
    Distributor distributor(rb);

    auto printer = std::make_unique<MockPrinter>();
    distributor.add_printer(std::move(printer));

    distributor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distributor.stop();

    return true;
}

// ==================== Distributor 分发测试 ====================

bool test_distributor_single_event() {
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
        return false;
    }

    return printer_ptr->get_events()[0].level == LogLevel::Info;
}

bool test_distributor_multiple_events() {
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

    return printer_ptr->get_write_count() == static_cast<size_t>(EVENT_COUNT);
}

bool test_distributor_multiple_printers() {
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

    return printer1_ptr->get_write_count() == 1 && printer2_ptr->get_write_count() == 1;
}

// ==================== Distributor 级别过滤测试 ====================

bool test_distributor_level_filtering() {
    RingBuffer rb(256);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Error);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

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

    return printer_ptr->get_write_count() == 2;
}

// ==================== Distributor 并发测试 ====================

bool test_distributor_concurrent_enqueue() {
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
    return printer_ptr->get_write_count() == static_cast<size_t>(expected);
}

// ==================== Distributor 生命周期测试 ====================

bool test_distributor_drain_on_stop() {
    RingBuffer rb(256);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    for (int i = 0; i < 50; ++i) {
        LogEvent event = create_test_event(LogLevel::Info, "Drain test");
        rb.enqueue(std::move(event));
    }

    distributor->stop();

    return printer_ptr->get_write_count() > 0;
}

bool test_distributor_flush_on_stop() {
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

    return printer_ptr->is_flushed();
}

bool test_distributor_double_start_stop() {
    RingBuffer rb(256);
    Distributor distributor(rb);

    distributor.start();
    distributor.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    distributor.stop();
    distributor.stop();

    return true;
}

// ==================== Distributor 批量处理测试 ====================

bool test_distributor_batch_processing() {
    RingBuffer rb(1024);
    auto distributor = std::make_unique<Distributor>(rb);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    const int EVENT_COUNT = 200;
    for (int i = 0; i < EVENT_COUNT; ++i) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "Batch %d", i);
        LogEvent event = create_test_event(LogLevel::Info, msg);
        rb.enqueue(std::move(event));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    distributor->stop();

    return printer_ptr->get_write_count() == static_cast<size_t>(EVENT_COUNT);
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Distributor Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    TestResult result;

    run_test("Distributor creation", test_distributor_creation, result);
    run_test("Distributor start/stop", test_distributor_start_stop, result);
    run_test("Distributor add printer", test_distributor_add_printer, result);
    run_test("Distributor single event", test_distributor_single_event, result);
    run_test("Distributor multiple events", test_distributor_multiple_events, result);
    run_test("Distributor multiple printers", test_distributor_multiple_printers, result);
    run_test("Distributor level filtering", test_distributor_level_filtering, result);
    run_test("Distributor concurrent enqueue", test_distributor_concurrent_enqueue, result);
    run_test("Distributor drain on stop", test_distributor_drain_on_stop, result);
    run_test("Distributor flush on stop", test_distributor_flush_on_stop, result);
    run_test("Distributor double start/stop", test_distributor_double_start_stop, result);
    run_test("Distributor batch processing", test_distributor_batch_processing, result);

    print_test_summary("Distributor Test Suite", result);
    return result.failed > 0 ? 1 : 0;
}
