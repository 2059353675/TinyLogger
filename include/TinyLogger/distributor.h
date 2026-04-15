#pragma once

#include "printer.h"
#include "ring_buffer.h"
#include "types.h"
#include <thread>

namespace TinyLogger {

static constexpr size_t LOG_LEVEL_COUNT = 5;

class Distributor
{
public:
    explicit Distributor(RingBuffer& rb);
    ~Distributor();

public:
    void start();
    void stop();
    void add_printer(std::unique_ptr<Printer> p);
    bool should_log(LogLevel lvl) const {
        return static_cast<uint8_t>(lvl) >= static_cast<uint8_t>(global_min_level_);
    }

private:
    void run();
    void drain_remaining();
    void flush_all();

private:
    RingBuffer& ring_buffer_;
    std::vector<std::unique_ptr<Printer>> printers_;
    LogLevel global_min_level_;

    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace TinyLogger