#pragma once

#include "printer.hpp"
#include "queue_registry.hpp"
#include "ring_buffer.hpp"
#include "types.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace tiny_logger {

static constexpr size_t LOG_LEVEL_COUNT = static_cast<size_t>(LogLevel::Count);

class Distributor
{
public:
    explicit Distributor(QueueRegistry& registry,
                         WaitStrategy strategy = WaitStrategy::Blocking,
                         std::chrono::microseconds sleep_interval = std::chrono::milliseconds(1));
    ~Distributor();

public:
    void start();
    void stop();
    void add_printer(std::unique_ptr<Printer> p);
    bool set_printer_min_level(PrinterType type, LogLevel level);
    LogLevel min_level() const {
        return global_min_level_;
    }
    bool should_log(LogLevel lvl) const {
        return static_cast<uint8_t>(lvl) >= static_cast<uint8_t>(global_min_level_);
    }

    /* 生产者边沿触发通知：唤醒 Distributor 处理新事件 */
    void notify_work();

    /* 空闲等待期间 run() 循环迭代次数（用于 Idle CPU 回归测试） */
    uint64_t loop_iterations() const {
        return loop_iterations_.load(std::memory_order_relaxed);
    }

private:
    void recalculate_min_level();
    void run();
    void wait_for_work(LogEvent* batch);
    bool process_available(LogEvent* batch);
    void drain_all();
    void flush_all();
    void process_event(LogEvent& event);

private:
    QueueRegistry& registry_;
    std::vector<RingBuffer*> queues_;
    std::vector<std::unique_ptr<Printer>> printers_;
    LogLevel global_min_level_;

    WaitStrategy wait_strategy_;
    std::chrono::microseconds sleep_interval_;

    std::atomic<bool> running_{false};
    std::thread worker_;

    // Blocking 唤醒原语
    std::mutex wake_mutex_;
    std::condition_variable wake_cv_;
    std::atomic<uint64_t> generation_{0};      // 单调递增唤醒代数
    std::atomic<uint64_t> loop_iterations_{0}; // 空闲测试用循环计数
};

} // namespace tiny_logger
