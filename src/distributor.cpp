#include "tiny_logger/distributor.h"
#include <chrono>
#include <thread>

namespace tiny_logger {

Distributor::Distributor(QueueRegistry& registry) : registry_(registry) {
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

    if (worker_.joinable()) {
        worker_.join();
    }

    flush_all();
}

void Distributor::add_printer(std::unique_ptr<Printer> p) {
    printers_.emplace_back(std::move(p));
    recalculate_min_level();
}

void Distributor::recalculate_min_level() {
    LogLevel min_level = LogLevel::Fatal;
    for (const auto& printer : printers_) {
        LogLevel printer_level = printer->min_level();
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
    constexpr size_t BATCH_SIZE = 64;
    LogEvent batch[BATCH_SIZE];

    constexpr size_t SNAPSHOT_REFRESH_INTERVAL = 512 - 1;
    size_t loop_count = 0;

    while (running_) {
        if ((loop_count & SNAPSHOT_REFRESH_INTERVAL) == 0) {
            queues_ = registry_.snapshot();
        }
        ++loop_count;

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

        if (!any_dequeued) {
            std::this_thread::yield();
        }
    }

    drain_all();
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