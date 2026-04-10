#include "TinyLogger/ring_buffer.h"

namespace TinyLogger {

RingBuffer::RingBuffer(size_t capacity)
    : capacity_(capacity),
      mask_(capacity_ - 1),
      buffer_(static_cast<Slot*>(operator new[](sizeof(Slot) * capacity_))),
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

bool RingBuffer::enqueue(LogEvent&& e) {
    size_t pos = write_pos_.load(std::memory_order_relaxed);

    while (true) {
        Slot& slot = buffer_[pos & mask_];
        size_t seq = slot.sequence.load(std::memory_order_acquire);

        if (seq == pos) {
            if (write_pos_.compare_exchange_weak(pos, pos + 1)) {
                slot.event = e;
                slot.sequence.store(pos + 1, std::memory_order_release);
                return true;
            }
        } else {
            return false;
        }
    }
}

bool RingBuffer::dequeue(LogEvent& e) {
    Slot& slot = buffer_[read_pos_ & mask_];
    size_t seq = slot.sequence.load(std::memory_order_acquire);

    if (seq != read_pos_ + 1) {
        // 没数据
        return false;
    }

    // 读取数据
    e = slot.event;

    // 标记该 slot 可复用
    slot.sequence.store(read_pos_ + capacity_, std::memory_order_release);

    ++read_pos_;
    return true;
}
} // namespace TinyLogger