#include "tiny_logger/distributor.hpp"
#include <chrono>
#include <thread>

namespace tiny_logger {

namespace {
constexpr size_t BATCH_SIZE = 64;
constexpr size_t SNAPSHOT_REFRESH_INTERVAL = 512 - 1;
} // namespace

Distributor::Distributor(QueueRegistry& registry, WaitStrategy strategy, std::chrono::microseconds sleep_interval)
    : registry_(registry),
      wait_strategy_(strategy),
      sleep_interval_(sleep_interval) {
}

Distributor::~Distributor() {
    stop();
}

void Distributor::start() {
    recalculate_min_level();

    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return;
    }

    queues_ = registry_.snapshot();

    worker_ = std::thread(&Distributor::run, this);
}

void Distributor::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return;
    }

    notify_work(); // 唤醒可能阻塞在条件变量上的工作线程

    if (worker_.joinable()) {
        worker_.join();
    }

    flush_all();
}

void Distributor::add_printer(std::unique_ptr<Printer> p) {
    printers_.emplace_back(std::move(p));
    recalculate_min_level();
}

void Distributor::notify_work() {
    generation_.fetch_add(1, std::memory_order_release);
    wake_cv_.notify_one();
}

void Distributor::recalculate_min_level() {
    auto min_level = LogLevel::Fatal;
    for (const auto& printer : printers_) {
        auto printer_level = printer->min_level();
        if (static_cast<uint8_t>(printer_level) < static_cast<uint8_t>(min_level)) {
            min_level = printer_level;
        }
    }
    global_min_level_ = min_level;
}

bool Distributor::set_printer_min_level(PrinterType type, LogLevel level) {
    for (auto& p : printers_) {
        if (p->type() == type) {
            p->set_min_level(level);
            recalculate_min_level();
            return true;
        }
    }
    return false;
}

void Distributor::run() {
    LogEvent batch[BATCH_SIZE];

    size_t loop_count = 0;

    while (running_) {
        if ((loop_count & SNAPSHOT_REFRESH_INTERVAL) == 0) {
            queues_ = registry_.snapshot();
        }
        ++loop_count;
        loop_iterations_.fetch_add(1, std::memory_order_relaxed);

        if (process_available(batch)) {
            continue;
        }

        wait_for_work(batch);

        // Blocking/Sleep：每次醒来都刷新快照，及时纳入新注册的队列
        if (wait_strategy_ == WaitStrategy::Blocking || wait_strategy_ == WaitStrategy::Sleep) {
            queues_ = registry_.snapshot();
        }
    }

    drain_all();
}

void Distributor::wait_for_work(LogEvent* batch) {
    switch (wait_strategy_) {
        case WaitStrategy::BusySpin:
            break;
        case WaitStrategy::Yield:
            std::this_thread::yield();
            break;
        case WaitStrategy::Sleep:
            std::this_thread::sleep_for(sleep_interval_);
            break;
        case WaitStrategy::Blocking: {
            std::unique_lock lock(wake_mutex_);
            const uint64_t local = generation_.load(std::memory_order_acquire);
            // 捕获代数后刷新快照再复查：任何被观察到的入队（bump 可见）
            // 其队列必然已注册且能被看到，必须处理而非入睡
            queues_ = registry_.snapshot();
            if (process_available(batch)) {
                return;
            }
            wake_cv_.wait(lock, [&] {
                return generation_.load(std::memory_order_acquire) != local ||
                       !running_.load(std::memory_order_acquire);
            });
            break;
        }
    }
}

bool Distributor::process_available(LogEvent* batch) {
    bool any_dequeued = false;

    for (auto* q : queues_) {
        size_t count = 0;
        while (count < BATCH_SIZE && q->dequeue(batch[count])) {
            ++count;
            any_dequeued = true;
        }

        for (size_t i = 0; i < count; ++i) {
            process_event(batch[i]);
        }
    }

    return any_dequeued;
}

void Distributor::drain_all() {
    for (RingBuffer* q : queues_) {
        LogEvent event;
        while (q->dequeue(event)) {
            process_event(event);
        }
    }
    flush_all();
}

void Distributor::process_event(LogEvent& event) {
    if (!should_log(event.level))
        return;

    for (auto& p : printers_) {
        if (!p->should_log(event.level))
            continue;
        try {
            p->write(event);
        } catch (const std::exception&) {
            p->increment_error_count();
        }
    }

    event.destroy();
}

void Distributor::flush_all() {
    for (auto& p : printers_) {
        p->flush();
    }
}

} // namespace tiny_logger
