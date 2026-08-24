#pragma once

#include "types.hpp"
#include <atomic>
#include <optional>

namespace tiny_logger
{
    /* 入队结果（用于边沿触发通知） */
    enum class EnqueueResult : uint8_t {
        Full,                    // 缓冲区已满，入队失败
        Success_EmptyToNonEmpty, // 入队成功，且队列由空变为非空（Distributor 应被唤醒）
        Success_NonEmptyToNonEmpty // 入队成功，队列原本已非空（无需唤醒）
    };

    class RingBuffer
    {
    public:
        explicit RingBuffer(size_t capacity, OverflowPolicy policy);
        ~RingBuffer();

    public:
        EnqueueResult enqueue(LogEvent&& e);
        bool dequeue(LogEvent& e);

    private:
        const size_t capacity_;
        const size_t mask_;
        const OverflowPolicy overflow_policy_;

        Slot* buffer_;

        size_t write_pos_{0}; // 写指针（仅生产者写）
        std::atomic<size_t> read_pos_{0}; // 读指针（消费者写，生产者松弛读取以检测空态）

        std::atomic<uint64_t> dropped_count_{0}; // 丢弃计数
    };
} // namespace tiny_logger