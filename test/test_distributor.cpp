#include "test_common.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <tiny_logger/distributor.hpp>
#include <tiny_logger/printer.hpp>
#include <tiny_logger/queue_registry.hpp>
#include <tiny_logger/ring_buffer.hpp>
#include <tiny_logger/types.hpp>

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

// ==================== 工具 ====================

bool run_for_all_strategies(const std::function<bool(WaitStrategy)>& scenario) {
    const WaitStrategy strategies[] = {WaitStrategy::BusySpin, WaitStrategy::Yield, WaitStrategy::Sleep,
                                       WaitStrategy::Blocking};
    for (auto s : strategies) {
        if (!scenario(s)) {
            return false;
        }
    }
    return true;
}

// 直接操作 RingBuffer 属底层 API：Blocking 下必须自行 notify（其他策略忽略）
void notify_low_level(Distributor& distributor, WaitStrategy strategy) {
    if (strategy == WaitStrategy::Blocking) {
        distributor.notify_work();
    }
}

// ==================== 基础测试 ====================

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
    Distributor distributor(registry, WaitStrategy::Blocking);

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

// ==================== 事件处理测试（覆盖全部策略） ====================

bool test_single_event(WaitStrategy strategy) {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry, strategy);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    LogEvent event = create_test_event(LogLevel::Info, "Test event");
    rb.enqueue(std::move(event));
    notify_low_level(*distributor, strategy);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    if (printer_ptr->get_write_count() != 1) {
        return false;
    }

    return printer_ptr->get_events()[0].level == LogLevel::Info;
}

bool test_multiple_events(WaitStrategy strategy) {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry, strategy);

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
    notify_low_level(*distributor, strategy);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    distributor->stop();

    return printer_ptr->get_write_count() == static_cast<size_t>(EVENT_COUNT);
}

bool test_multiple_printers(WaitStrategy strategy) {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry, strategy);

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
    notify_low_level(*distributor, strategy);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    return printer1_ptr->get_write_count() == 1 && printer2_ptr->get_write_count() == 1;
}

bool test_level_filtering(WaitStrategy strategy) {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry, strategy);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Error);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    rb.enqueue(create_test_event(LogLevel::Debug, "Debug"));
    rb.enqueue(create_test_event(LogLevel::Info, "Info"));
    rb.enqueue(create_test_event(LogLevel::Error, "Error"));
    rb.enqueue(create_test_event(LogLevel::Fatal, "Fatal"));
    notify_low_level(*distributor, strategy);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    return printer_ptr->get_write_count() == 2;
}

bool test_drain_on_stop(WaitStrategy strategy) {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry, strategy);

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

bool test_flush_on_stop(WaitStrategy strategy) {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry, strategy);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor->add_printer(std::move(printer));

    distributor->start();

    LogEvent event = create_test_event(LogLevel::Info, "Flush test");
    rb.enqueue(std::move(event));
    notify_low_level(*distributor, strategy);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    distributor->stop();

    return printer_ptr->is_flushed();
}

bool test_batch_processing(WaitStrategy strategy) {
    QueueRegistry registry;
    RingBuffer rb(1024, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry, strategy);

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
    notify_low_level(*distributor, strategy);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    distributor->stop();

    return printer_ptr->get_write_count() == static_cast<size_t>(EVENT_COUNT);
}

bool test_printer_exception_handling(WaitStrategy strategy) {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    auto distributor = std::make_unique<Distributor>(registry, strategy);

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
    notify_low_level(*distributor, strategy);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor->stop();

    return printer_ptr->error_count() > 0;
}

// ==================== Blocking 专属行为测试 ====================

bool test_blocking_new_queue_after_start() {
    QueueRegistry registry;
    Distributor distributor(registry, WaitStrategy::Blocking);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor.add_printer(std::move(printer));

    distributor.start();

    // start 之后注册新队列（模拟新线程惰性注册）
    auto rb = std::make_unique<RingBuffer>(256, OverflowPolicy::Discard);
    registry.register_queue(rb.get());

    LogEvent event = create_test_event(LogLevel::Info, "late queue");
    rb->enqueue(std::move(event));
    distributor.notify_work();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    distributor.stop();

    return printer_ptr->get_write_count() == 1;
}

// ==================== Idle CPU 回归测试 ====================

bool test_idle_cpu_blocking() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    Distributor distributor(registry, WaitStrategy::Blocking);
    distributor.add_printer(std::make_unique<MockPrinter>());

    distributor.start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    distributor.stop();

    // 纯阻塞：无 notify 时几乎不迭代（≈1），防止未来改回忙轮询
    return distributor.loop_iterations() < 10;
}

bool test_idle_cpu_yield() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    Distributor distributor(registry, WaitStrategy::Yield);
    distributor.add_printer(std::make_unique<MockPrinter>());

    distributor.start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    distributor.stop();

    // Yield 忙轮询：2 秒内应迭代成千上万次
    return distributor.loop_iterations() > 1000;
}

bool test_idle_cpu_sleep() {
    QueueRegistry registry;
    RingBuffer rb(256, OverflowPolicy::Discard);
    registry.register_queue(&rb);
    Distributor distributor(registry, WaitStrategy::Sleep, std::chrono::milliseconds(1));
    distributor.add_printer(std::make_unique<MockPrinter>());

    const auto start = std::chrono::steady_clock::now();
    distributor.start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    distributor.stop();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    // Sleep 1ms：理论迭代率约 1 次/ms。上界按实测耗时自校准，用于排除忙轮询回归
    // （忙轮询会在同样时间内迭代数千倍，必然超出）；下界只需确认循环确实在运行。
    // 平台 sleep 精度差异（如 macOS 定时器合并将 1ms 睡眠拉长到数 ms~十余 ms）只会
    // 降低迭代数，不影响忙轮询判定，因此不做绝对下界限制。
    const uint64_t n = distributor.loop_iterations();
    const uint64_t max_iterations = static_cast<uint64_t>(elapsed_ms) * 20; // 理论 ~1/ms 的 20 倍余量
    return n > 0 && n <= max_iterations;
}

// ==================== 压力测试（notify/wait/stop 竞争） ====================

bool test_stress_concurrent_producers(WaitStrategy strategy) {
    constexpr int THREADS = 4;
    constexpr int EVENTS = 2000;
    constexpr size_t QUEUE_CAP = 4096;
    constexpr int64_t TOTAL = THREADS * EVENTS;

    QueueRegistry registry;
    Distributor distributor(registry, strategy);

    auto printer = std::make_unique<MockPrinter>();
    printer->set_min_level(LogLevel::Debug);
    MockPrinter* printer_ptr = printer.get();
    distributor.add_printer(std::move(printer));

    // 队列由测试持有，必须比 Distributor 存活更久（模拟 Logger 持有队列）
    std::vector<std::unique_ptr<RingBuffer>> queues;
    for (int t = 0; t < THREADS; ++t) {
        queues.push_back(std::make_unique<RingBuffer>(QUEUE_CAP, OverflowPolicy::Discard));
    }

    distributor.start();

    // start 之后注册（模拟惰性注册），容量 > 事件数，无溢出
    for (int t = 0; t < THREADS; ++t) {
        registry.register_queue(queues[t].get());
    }

    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < EVENTS; ++i) {
                LogEvent event = create_test_event(LogLevel::Info, "stress");
                if (queues[t]->enqueue(std::move(event)) == EnqueueResult::Success_EmptyToNonEmpty) {
                    distributor.notify_work();
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    // 不 stop，等待 notify 驱动的投递完成
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (printer_ptr->get_write_count() < static_cast<size_t>(TOTAL)) {
        if (std::chrono::steady_clock::now() > deadline) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    const bool ok = printer_ptr->get_write_count() == static_cast<size_t>(TOTAL);
    distributor.stop();
    return ok;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Distributor Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    TestResult result;

    run_test("Distributor creation", test_distributor_creation, result);
    run_test("Distributor start/stop", test_distributor_start_stop, result);
    run_test("Distributor add printer", test_distributor_add_printer, result);
    run_test("Distributor double start/stop", test_distributor_double_start_stop, result);

    run_test("Single event (all strategies)", [] { return run_for_all_strategies(test_single_event); }, result, 3);
    run_test("Multiple events (all strategies)", [] { return run_for_all_strategies(test_multiple_events); }, result, 3);
    run_test("Multiple printers (all strategies)", [] { return run_for_all_strategies(test_multiple_printers); }, result, 3);
    run_test("Level filtering (all strategies)", [] { return run_for_all_strategies(test_level_filtering); }, result, 3);
    run_test("Drain on stop (all strategies)", [] { return run_for_all_strategies(test_drain_on_stop); }, result, 3);
    run_test("Flush on stop (all strategies)", [] { return run_for_all_strategies(test_flush_on_stop); }, result, 3);
    run_test("Batch processing (all strategies)", [] { return run_for_all_strategies(test_batch_processing); }, result, 3);
    run_test("Printer exception handling (all strategies)",
             [] { return run_for_all_strategies(test_printer_exception_handling); }, result, 3);

    run_test("Blocking: new queue after start", test_blocking_new_queue_after_start, result, 3);

    run_test("Idle CPU: Blocking (regression)", test_idle_cpu_blocking, result, 3);
    run_test("Idle CPU: Yield (regression)", test_idle_cpu_yield, result, 3);
    run_test("Idle CPU: Sleep (regression)", test_idle_cpu_sleep, result, 3);

    run_test("Stress: concurrent producers (all strategies)",
             [] { return run_for_all_strategies(test_stress_concurrent_producers); }, result, 3);

    print_test_summary("Distributor Test Suite", result);
    return result.failed > 0 ? 1 : 0;
}
