#include "tiny_logger/ring_buffer.hpp"
#include <cassert>

namespace tiny_logger {

RingBuffer::RingBuffer(size_t capacity, OverflowPolicy policy)
    : capacity_(capacity),
      mask_(capacity - 1),
      overflow_policy_(policy),
      buffer_(static_cast<Slot*>(operator new[](sizeof(Slot) * capacity))),
      write_pos_(0),
      read_pos_(0) {
    assert((capacity & (capacity - 1)) == 0 && "capacity must be a power of 2");
    for (size_t i = 0; i < capacity_; ++i) {
        new (&buffer_[i]) Slot();
        buffer_[i].sequence.store(i, std::memory_order_relaxed);
    }
}

RingBuffer::~RingBuffer() {
    for (size_t i = 0; i < capacity_; ++i) {
        buffer_[i].~Slot();
    }
    operator delete[](buffer_);
}

EnqueueResult RingBuffer::enqueue(LogEvent&& e) {
    size_t pos = write_pos_;
    Slot& slot = buffer_[pos & mask_];
    size_t seq = slot.sequence.load(std::memory_order_acquire);

    if (seq != pos) {
        return EnqueueResult::Full;
    }

    bool was_empty = (read_pos_.load(std::memory_order_relaxed) == pos);

    slot.event = std::move(e);
    slot.sequence.store(pos + 1, std::memory_order_release);
    ++write_pos_;

    return was_empty ? EnqueueResult::Success_EmptyToNonEmpty
                     : EnqueueResult::Success_NonEmptyToNonEmpty;
}

bool RingBuffer::dequeue(LogEvent& e) {
    size_t pos = read_pos_.load(std::memory_order_relaxed);
    Slot& slot = buffer_[pos & mask_];
    size_t seq = slot.sequence.load(std::memory_order_acquire);

    if (seq != pos + 1) {
        return false;
    }

    e = std::move(slot.event);
    slot.sequence.store(pos + capacity_, std::memory_order_release);

    read_pos_.store(pos + 1, std::memory_order_release);
    return true;
}
} // namespace tiny_logger