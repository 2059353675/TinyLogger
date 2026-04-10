#pragma once

#include "types.h"
#include <atomic>
#include <cassert>
#include <optional>

namespace TinyLogger {

class RingBuffer
{
public:
    explicit RingBuffer(size_t capacity);
    ~RingBuffer();

public:
    bool enqueue(LogEvent&& e);
    bool dequeue(LogEvent& e);

private:
    const size_t capacity_;
    const size_t mask_; // capacity_ - 1，用于快速取模
    const OverflowPolicy overflow_policy_ = OverflowPolicy::Discard;

    Slot* buffer_;

    std::atomic<size_t> write_pos_{0}; // 写指针
    size_t read_pos_{0};               // 读指针

    std::atomic<uint64_t> dropped_count_{0}; // 丢弃计数
};

} // namespace TinyLogger