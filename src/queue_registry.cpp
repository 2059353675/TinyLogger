#include "tiny_logger/queue_registry.h"

namespace tiny_logger {

void QueueRegistry::register_queue(RingBuffer* q) {
    std::lock_guard<std::mutex> lock(mutex_);
    queues_.push_back(q);
}

std::vector<RingBuffer*> QueueRegistry::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queues_;
}

} // namespace tiny_logger