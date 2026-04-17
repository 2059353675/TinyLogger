#include "test_common.h"
#include <TinyLogger/ring_buffer.h>
#include <TinyLogger/types.h>

using namespace tiny_logger;
using namespace tiny_logger::test;

// ==================== 基础测试 ====================

bool test_ring_buffer_creation() {
    try {
        RingBuffer rb(256);
        return true;
    } catch (...) {
        return false;
    }
}

bool test_single_enqueue_dequeue() {
    RingBuffer rb(256);

    LogEvent event;
    create_test_event(event, LogLevel::Info, "Hello, World!");

    if (!rb.enqueue(std::move(event))) {
        return false;
    }

    LogEvent dequeued;
    if (!rb.dequeue(dequeued)) {
        return false;
    }

    return dequeued.level == LogLevel::Info && std::strncmp(dequeued.preformatted, "Hello, World!", dequeued.length) == 0;
}

bool test_empty_buffer_dequeue() {
    RingBuffer rb(256);

    LogEvent event;
    return !rb.dequeue(event);
}

bool test_buffer_capacity() {
    RingBuffer rb(4); // 小容量测试

    // 填充缓冲区
    for (size_t i = 0; i < 4; ++i) {
        LogEvent event;
        char msg[32];
        std::snprintf(msg, sizeof(msg), "Message %zu", i);
        create_test_event(event, LogLevel::Debug, msg);

        if (!rb.enqueue(std::move(event))) {
            return false;
        }
    }

    // 第5个应该失败
    LogEvent overflow_event;
    create_test_event(overflow_event, LogLevel::Error, "Overflow");
    return !rb.enqueue(std::move(overflow_event));
}

bool test_fifo_ordering() {
    RingBuffer rb(16);

    const char* messages[] = {"First", "Second", "Third", "Fourth"};
    size_t count = sizeof(messages) / sizeof(messages[0]);

    for (size_t i = 0; i < count; ++i) {
        LogEvent event;
        create_test_event(event, LogLevel::Info, messages[i]);
        rb.enqueue(std::move(event));
    }

    for (size_t i = 0; i < count; ++i) {
        LogEvent event;
        if (!rb.dequeue(event)) {
            return false;
        }

        if (std::strncmp(event.preformatted, messages[i], event.length) != 0) {
            return false;
        }
    }

    return true;
}

// ==================== 多级测试 ====================

bool test_multiple_log_levels() {
    RingBuffer rb(64);

    LogLevel levels[] = {LogLevel::Debug, LogLevel::Info, LogLevel::Error, LogLevel::Fatal};

    for (auto level : levels) {
        LogEvent event;
        create_test_event(event, level, "Test message");
        rb.enqueue(std::move(event));
    }

    for (auto expected_level : levels) {
        LogEvent event;
        if (!rb.dequeue(event)) {
            return false;
        }

        if (event.level != expected_level) {
            return false;
        }
    }

    return true;
}

bool test_large_message() {
    RingBuffer rb(256);

    char large_msg[LOG_MSG_SIZE - 1];
    std::memset(large_msg, 'A', sizeof(large_msg) - 1);
    large_msg[sizeof(large_msg) - 1] = '\0';

    LogEvent event;
    create_test_event(event, LogLevel::Info, large_msg);

    if (!rb.enqueue(std::move(event))) {
        return false;
    }

    LogEvent dequeued;
    if (!rb.dequeue(dequeued)) {
        return false;
    }

    return dequeued.length == LOG_MSG_SIZE - 2;
}

// ==================== 并发测试 ====================

bool test_concurrent_single_producer_consumer() {
    constexpr size_t BUFFER_SIZE = 1024;
    constexpr size_t ITEM_COUNT = 10000;

    RingBuffer rb(BUFFER_SIZE);
    std::atomic<size_t> produced{0};
    std::atomic<size_t> consumed{0};
    std::atomic<bool> producer_done{false};

    // 生产者线程
    std::thread producer([&]() {
        for (size_t i = 0; i < ITEM_COUNT; ++i) {
            LogEvent event;
            char msg[64];
            std::snprintf(msg, sizeof(msg), "Message %zu", i);
            create_test_event(event, LogLevel::Info, msg);

            while (!rb.enqueue(std::move(event))) {
                std::this_thread::yield();
            }
            produced.fetch_add(1);
        }
        producer_done.store(true);
    });

    // 消费者线程
    std::thread consumer([&]() {
        while (true) {
            LogEvent event;
            if (rb.dequeue(event)) {
                consumed.fetch_add(1);
            } else if (producer_done.load() && consumed.load() == ITEM_COUNT) {
                break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    return produced.load() == ITEM_COUNT && consumed.load() == ITEM_COUNT;
}

bool test_concurrent_multi_producer_single_consumer() {
    constexpr size_t BUFFER_SIZE = 1024;
    constexpr size_t PRODUCER_COUNT = 4;
    constexpr size_t ITEMS_PER_PRODUCER = 5000;
    constexpr size_t TOTAL_ITEMS = PRODUCER_COUNT * ITEMS_PER_PRODUCER;

    RingBuffer rb(BUFFER_SIZE);
    std::atomic<size_t> consumed{0};
    std::atomic<size_t> active_producers{PRODUCER_COUNT};

    // 多个生产者
    std::vector<std::thread> producers;
    for (size_t p = 0; p < PRODUCER_COUNT; ++p) {
        producers.emplace_back([&, p]() {
            for (size_t i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                LogEvent event;
                char msg[64];
                std::snprintf(msg, sizeof(msg), "P%zu-I%zu", p, i);
                create_test_event(event, LogLevel::Info, msg);

                while (!rb.enqueue(std::move(event))) {
                    std::this_thread::yield();
                }
            }
            active_producers.fetch_sub(1);
        });
    }

    // 消费者
    std::thread consumer([&]() {
        while (true) {
            LogEvent event;
            if (rb.dequeue(event)) {
                consumed.fetch_add(1);
            } else if (active_producers.load() == 0 && consumed.load() == TOTAL_ITEMS) {
                break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    for (auto& t : producers) {
        t.join();
    }
    consumer.join();

    return consumed.load() == TOTAL_ITEMS;
}

// ==================== 边界测试 ====================

bool test_power_of_two_capacities() {
    size_t capacities[] = {2, 4, 8, 16, 32, 64, 128, 256};

    for (size_t cap : capacities) {
        try {
            RingBuffer rb(cap);

            // 测试能否填满
            for (size_t i = 0; i < cap; ++i) {
                LogEvent event;
                create_test_event(event, LogLevel::Debug, "test");
                if (!rb.enqueue(std::move(event))) {
                    return false;
                }
            }
        } catch (...) {
            return false;
        }
    }

    return true;
}

bool test_wraparound() {
    RingBuffer rb(8);

    // 填充并消费多次，测试循环
    for (int round = 0; round < 10; ++round) {
        for (size_t i = 0; i < 8; ++i) {
            LogEvent event;
            char msg[32];
            std::snprintf(msg, sizeof(msg), "R%d-I%zu", round, i);
            create_test_event(event, LogLevel::Info, msg);
            rb.enqueue(std::move(event));
        }

        for (size_t i = 0; i < 8; ++i) {
            LogEvent event;
            if (!rb.dequeue(event)) {
                return false;
            }
        }
    }

    return true;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  RingBuffer Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    TestResult result;

    // 基础测试
    run_test("RingBuffer creation", test_ring_buffer_creation, result);
    run_test("Single enqueue and dequeue", test_single_enqueue_dequeue, result);
    run_test("Empty buffer dequeue returns false", test_empty_buffer_dequeue, result);
    run_test("Buffer capacity enforcement", test_buffer_capacity, result);
    run_test("FIFO ordering", test_fifo_ordering, result);

    // 多级测试
    run_test("Multiple log levels", test_multiple_log_levels, result);
    run_test("Large message (near LOG_MSG_SIZE)", test_large_message, result);

    // 并发测试
    run_test("Concurrent single producer-consumer", test_concurrent_single_producer_consumer, result);
    run_test("Concurrent multi-producer single consumer", test_concurrent_multi_producer_single_consumer, result);

    // 边界测试
    run_test("Power-of-2 capacities", test_power_of_two_capacities, result);
    run_test("Buffer wraparound", test_wraparound, result);

    print_test_summary("RingBuffer Test Suite", result);

    return result.failed > 0 ? 1 : 0;
}
