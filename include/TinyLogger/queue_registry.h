#pragma once

#include "ring_buffer.h"
#include <mutex>
#include <vector>

namespace tiny_logger {

class QueueRegistry
{
public:
    void register_queue(RingBuffer* q);

    std::vector<RingBuffer*> snapshot() const;

private:
    mutable std::mutex mutex_;
    std::vector<RingBuffer*> queues_;
};

} // namespace tiny_logger