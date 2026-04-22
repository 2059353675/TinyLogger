#include "TinyLogger/ring_buffer.h"
#include <cstring>

namespace tiny_logger {

RingBuffer::RingBuffer(size_t capacity, OverflowPolicy policy)
    : capacity_(capacity),
      mask_(capacity_ - 1),
      overflow_policy_(policy),
      buffer_(static_cast<Slot*>(operator new[](sizeof(Slot) * capacity_))),
      write_pos_(0),
      read_pos_(0) {
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
    /**
     * Disruptor 风格无锁队列算法:
     * 1. 读取当前写位置
     * 2. 检查槽位 sequence 是否等于写位置(表明槽位归生产者所有)
     * 3. 若相等则 CAS 更新写位置,复制数据,标记槽位可读
     * 4. 否则返回 false(队列满)
     */
    size_t pos = write_pos_;
    Slot& slot = buffer_[pos & mask_];
    size_t seq = slot.sequence.load(std::memory_order_acquire);

    if (seq != pos) {
        return false;
    }

    slot.event = std::move(e);
    slot.sequence.store(pos + 1, std::memory_order_release);
    ++write_pos_;
    return true;
}

bool RingBuffer::dequeue(LogEvent& e) {
    size_t pos = read_pos_;
    Slot& slot = buffer_[pos & mask_];
    size_t seq = slot.sequence.load(std::memory_order_acquire);

    if (seq != pos + 1) {
        // 没数据
        return false;
    }

    // 读取数据
    e = std::move(slot.event);

    // 标记该 slot 可复用
    slot.sequence.store(pos + capacity_, std::memory_order_release);

    ++read_pos_;
    return true;
}
} // namespace tiny_logger