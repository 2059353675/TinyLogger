#include <TinyLogger/ring_buffer.h>
#include <TinyLogger/types.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

using namespace TinyLogger;

// ==================== 工具函数 ====================

static bool create_test_event(LogEvent& event, LogLevel level, const char* msg) {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
    
    size_t len = std::strlen(msg);
    if (len >= LOG_MSG_SIZE) {
        return false;
    }
    
    event = LogEvent(level, ts, msg, len);
    return true;
}

// ==================== 基础测试 ====================

bool test_ring_buffer_creation() {
    std::cout << "[TEST] RingBuffer creation... ";
    try {
        RingBuffer rb(256);
        std::cout << "PASSED" << std::endl;
        return true;
    } catch (...) {
        std::cout << "FAILED" << std::endl;
        return false;
    }
}

bool test_single_enqueue_dequeue() {
    std::cout << "[TEST] Single enqueue and dequeue... ";
    RingBuffer rb(256);
    
    LogEvent event;
    create_test_event(event, LogLevel::Info, "Hello, World!");
    
    if (!rb.enqueue(std::move(event))) {
        std::cout << "FAILED (enqueue)" << std::endl;
        return false;
    }
    
    LogEvent dequeued;
    if (!rb.dequeue(dequeued)) {
        std::cout << "FAILED (dequeue)" << std::endl;
        return false;
    }
    
    if (dequeued.level != LogLevel::Info || 
        std::strncmp(dequeued.buffer, "Hello, World!", dequeued.length) != 0) {
        std::cout << "FAILED (data mismatch)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_empty_buffer_dequeue() {
    std::cout << "[TEST] Empty buffer dequeue returns false... ";
    RingBuffer rb(256);
    
    LogEvent event;
    if (rb.dequeue(event)) {
        std::cout << "FAILED (should return false)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_buffer_capacity() {
    std::cout << "[TEST] Buffer capacity enforcement... ";
    RingBuffer rb(4); // 小容量测试
    
    // 填充缓冲区
    for (size_t i = 0; i < 4; ++i) {
        LogEvent event;
        char msg[32];
        std::snprintf(msg, sizeof(msg), "Message %zu", i);
        create_test_event(event, LogLevel::Debug, msg);
        
        if (!rb.enqueue(std::move(event))) {
            std::cout << "FAILED (enqueue " << i << ")" << std::endl;
            return false;
        }
    }
    
    // 第5个应该失败
    LogEvent overflow_event;
    create_test_event(overflow_event, LogLevel::Error, "Overflow");
    if (rb.enqueue(std::move(overflow_event))) {
        std::cout << "FAILED (should reject overflow)" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_fifo_ordering() {
    std::cout << "[TEST] FIFO ordering... ";
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
            std::cout << "FAILED (dequeue " << i << ")" << std::endl;
            return false;
        }
        
        if (std::strncmp(event.buffer, messages[i], event.length) != 0) {
            std::cout << "FAILED (order mismatch at " << i << ")" << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 多级测试 ====================

bool test_multiple_log_levels() {
    std::cout << "[TEST] Multiple log levels... ";
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
            std::cout << "FAILED (missing event)" << std::endl;
            return false;
        }
        
        if (event.level != expected_level) {
            std::cout << "FAILED (level mismatch)" << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_large_message() {
    std::cout << "[TEST] Large message (near LOG_MSG_SIZE)... ";
    RingBuffer rb(256);
    
    char large_msg[LOG_MSG_SIZE - 1];
    std::memset(large_msg, 'A', sizeof(large_msg) - 1);
    large_msg[sizeof(large_msg) - 1] = '\0';
    
    LogEvent event;
    create_test_event(event, LogLevel::Info, large_msg);
    
    if (!rb.enqueue(std::move(event))) {
        std::cout << "FAILED (enqueue)" << std::endl;
        return false;
    }
    
    LogEvent dequeued;
    if (!rb.dequeue(dequeued)) {
        std::cout << "FAILED (dequeue)" << std::endl;
        return false;
    }
    
    if (dequeued.length != LOG_MSG_SIZE - 2) {
        std::cout << "FAILED (length mismatch: " << dequeued.length << ")" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 并发测试 ====================

bool test_concurrent_single_producer_consumer() {
    std::cout << "[TEST] Concurrent single producer-consumer... ";
    
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
    
    if (produced.load() != ITEM_COUNT || consumed.load() != ITEM_COUNT) {
        std::cout << "FAILED (produced: " << produced.load() 
                  << ", consumed: " << consumed.load() << ")" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_concurrent_multi_producer_single_consumer() {
    std::cout << "[TEST] Concurrent multi-producer single consumer... ";
    
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
    
    if (consumed.load() != TOTAL_ITEMS) {
        std::cout << "FAILED (expected: " << TOTAL_ITEMS 
                  << ", got: " << consumed.load() << ")" << std::endl;
        return false;
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 边界测试 ====================

bool test_power_of_two_capacities() {
    std::cout << "[TEST] Power-of-2 capacities (2, 4, 8, 16, 32)... ";
    
    size_t capacities[] = {2, 4, 8, 16, 32, 64, 128, 256};
    
    for (size_t cap : capacities) {
        try {
            RingBuffer rb(cap);
            
            // 测试能否填满
            for (size_t i = 0; i < cap; ++i) {
                LogEvent event;
                create_test_event(event, LogLevel::Debug, "test");
                if (!rb.enqueue(std::move(event))) {
                    std::cout << "FAILED (capacity " << cap << ", enqueue " << i << ")" << std::endl;
                    return false;
                }
            }
        } catch (...) {
            std::cout << "FAILED (capacity " << cap << " exception)" << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

bool test_wraparound() {
    std::cout << "[TEST] Buffer wraparound... ";
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
                std::cout << "FAILED (round " << round << ", dequeue " << i << ")" << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  RingBuffer Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    auto run_test = [&](bool (*test_func)()) {
        if (test_func()) {
            passed++;
        } else {
            failed++;
        }
    };
    
    // 基础测试
    run_test(test_ring_buffer_creation);
    run_test(test_single_enqueue_dequeue);
    run_test(test_empty_buffer_dequeue);
    run_test(test_buffer_capacity);
    run_test(test_fifo_ordering);
    
    // 多级测试
    run_test(test_multiple_log_levels);
    run_test(test_large_message);
    
    // 并发测试
    run_test(test_concurrent_single_producer_consumer);
    run_test(test_concurrent_multi_producer_single_consumer);
    
    // 边界测试
    run_test(test_power_of_two_capacities);
    run_test(test_wraparound);
    
    std::cout << "========================================" << std::endl;
    std::cout << "  Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
