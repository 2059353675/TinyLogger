#include "TinyLogger/distributor.h"
#include <chrono>
#include <thread>

namespace TinyLogger {

Distributor::Distributor(RingBuffer& rb) : ring_buffer_(rb) {
}

Distributor::~Distributor() {
    stop();
}

void Distributor::start() {
    LogLevel min_level = LogLevel::Fatal;
    for (const auto& printer : printers_) {
        LogLevel printer_level = printer->min_level();
        if (static_cast<uint8_t>(printer_level) < static_cast<uint8_t>(min_level)) {
            min_level = printer_level;
        }
    }
    global_min_level_ = min_level;

    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return; // already running
    }

    worker_ = std::thread(&Distributor::run, this);
}

void Distributor::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return; // already stopped
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    flush_all();
}

void Distributor::add_printer(std::unique_ptr<Printer> p) {
    printers_.emplace_back(std::move(p));
}

void Distributor::run() {
    constexpr size_t BATCH_SIZE = 64;

    LogEvent batch[BATCH_SIZE];

    while (running_) {
        size_t count;

        // 量取数据
        for (count = 0; count < BATCH_SIZE; ++count) {
            if (!ring_buffer_.dequeue(batch[count])) {
                break;
            }
        }

        // 无数据则让出线程
        if (count == 0) {
            std::this_thread::yield();
            continue;
        }

        // 分发到 printers
        for (size_t i = 0; i < count; ++i) {
            const LogEvent& event = batch[i];
            if (!should_log(event.level))
                continue;
            for (auto& p : printers_) {
                if (!p->should_log(event.level))
                    continue;
                try {
                    p->write(event);
                } catch (const std::exception&) {
                    p->increment_error_count();
                }
            }
        }
    }

    drain_remaining();
}

void Distributor::drain_remaining() {
    LogEvent event;

    while (ring_buffer_.dequeue(event)) {
        if (!should_log(event.level))
            continue;
        for (auto& p : printers_) {
            if (!p->should_log(event.level))
                continue;
            try {
                p->write(event);
            } catch (const std::exception&) {
                p->increment_error_count();
            }
        }
    }

    flush_all();
}

void Distributor::flush_all() {
    for (auto& p : printers_) {
        p->flush();
    }
}

} // namespace TinyLogger