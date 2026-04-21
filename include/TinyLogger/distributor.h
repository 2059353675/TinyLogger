#pragma once

#include "printer.h"
#include "queue_registry.h"
#include "ring_buffer.h"
#include "types.h"
#include <thread>
#include <vector>

namespace tiny_logger {

static constexpr size_t LOG_LEVEL_COUNT = 5;

class Distributor
{
public:
    explicit Distributor(QueueRegistry& registry);
    ~Distributor();

public:
    void start();
    void stop();
    void add_printer(std::unique_ptr<Printer> p);
    bool set_min_level(PrinterType type, LogLevel level);
    bool set_printer_min_level(PrinterType type, LogLevel level);
    LogLevel min_level() const {
        return global_min_level_;
    }
    bool should_log(LogLevel lvl) const {
        return static_cast<uint8_t>(lvl) >= static_cast<uint8_t>(global_min_level_);
    }

private:
    void recalculate_min_level();
    void run();
    void drain_all();
    void flush_all();
    void process_event(LogEvent& event);

private:
    QueueRegistry& registry_;
    std::vector<RingBuffer*> queues_;
    std::vector<std::unique_ptr<Printer>> printers_;
    LogLevel global_min_level_;

    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace tiny_logger