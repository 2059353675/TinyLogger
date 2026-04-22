#include "test_common.h"
#include <TinyLogger/distributor.h>
#include <TinyLogger/printer.h>
#include <TinyLogger/queue_registry.h>
#include <TinyLogger/ring_buffer.h>
#include <TinyLogger/types.h>
#include <mutex>

using namespace tiny_logger;
using namespace tiny_logger::test;

class MockPrinter : public Printer
{
public:
    MockPrinter() : write_count_(0), should_throw_(false) {
        min_level_ = LogLevel::Debug;
    }

    void write(LogEvent& event) override {
        if (!should_log(event.level))
            return;
        if (should_throw_) {
            throw std::runtime_error("Mock printer error");
        }
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(std::move(event));
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

    bool is_flushed() const {
        return flushed_;
    }

    void set_min_level(LogLevel level) {
        min_level_ = level;
    }

    void set_throw_on_write(bool should_throw) {
        should_throw_ = should_throw;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.clear();
        write_count_.store(0);
        flushed_ = false;
        error_count_.store(0);
        should_throw_ = false;
    }

private:
    std::mutex mutex_;
    std::vector<LogEvent> events_;
    std::atomic<size_t> write_count_;
    bool flushed_ = false;
    bool should_throw_;
};

bool test_distributor_creation() {
    try {
        QueueRegistry registry;
        RingBuffer rb(256, OverflowPolicy::Discard);
        registry.register_queue(&rb);
        Distributor distributor(registry);
        return true;
    } catch (...) {
        return false;
    }
}

bool test_distributor_start_stop() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    Distributor distributor(registry);

    distributor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distributor.stop();

    return true;
}

bool test_distributor_add_printer() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    Distributor distributor(registry);

    auto printer = std::make_unique<MockPrinter>();
    distributor.add_printer(std::move(printer));

    distributor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distributor.stop();

    return true;
}

bool test_distributor_single_event() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
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
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
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
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer1 = std::make_unique<MockPrinter>();
    printer1->set_min_level(LogLevel::Debug);
    MockPrinter* printer1_ptr = printer1.get();
    distributor->add_printer(std::move(printer1));

    auto printer2 = std::make_unique<MockPrinter>();
    printer2->set_min_level(LogLevel::Info);
    MockPrinter* printer2_ptr = printer2.get();
    distributor->add_printer(std::move(printer2));

    distributor->start();

    LogEvent event = create_test_event(LogLevel::Error, "Multi-printer test");
    rb.enqueue(std::move(event));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    return printer1_ptr->get_write_count() == 1 && printer2_ptr->get_write_count() == 1;
}

bool test_distributor_level_filtering() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Error);
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

bool test_distributor_concurrent_enqueue() {
    QueueRegistry registry;
    RingBuffer rb(1024, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
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

bool test_distributor_drain_on_stop() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
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
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
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
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    Distributor distributor(registry);

    distributor.start();
    distributor.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    distributor.stop();
    distributor.stop();

    return true;
}

bool test_distributor_batch_processing() {
    QueueRegistry registry;
    RingBuffer rb(1024, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
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

bool test_distributor_printer_exception_handling() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
    printer->set_throw_on_write(true);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    for (int i = 0; i < 10; ++i) {
        LogEvent event = create_test_event(LogLevel::Info, "Test");
        rb.enqueue(std::move(event));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    return printer_ptr->error_count() > 0;
}

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
    run_test("Distributor printer exception handling", test_distributor_printer_exception_handling, result);

    print_test_summary("Distributor Test Suite", result);
    return result.failed > 0 ? 1 : 0;
}