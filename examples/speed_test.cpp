/**
 * TinyLogger 性能测试 - 精确测量各环节开销
 *
 * 测试内容：
 *   1. 延迟测试：分别测量 tuple构造、RingBuffer::enqueue、logger.info() 全流程
 *   2. 吞吐量测试：固定时间窗口内的日志处理能力
 *   3. 并发测试：多线程同时写入 RingBuffer
 *
 * 编译（使用 CMake 构建）：
 *   mkdir build && cd build && cmake .. && make
 */

#include <TinyLogger/logger_builder.h>
#include <TinyLogger/printer_null.h>
#include <TinyLogger/ring_buffer.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

namespace {

constexpr size_t BUFFER_SIZE = 256;
constexpr int WARMUP_ITERATIONS = 10000;
constexpr int MEASURE_ITERATIONS = 50000;
constexpr int THROUGHPUT_DURATION_MS = 1000;
constexpr int CONCURRENT_THREADS[] = {1, 2, 4, 8};

struct LatencyStats {
    double avg_ns = 0;
    double min_ns = 0;
    double max_ns = 0;
    double p50_ns = 0;
    double p90_ns = 0;
    double p99_ns = 0;
    double p999_ns = 0;
    double stddev_ns = 0;
};

struct ThroughputStats {
    double ops_per_sec = 0;
    size_t total_ops = 0;
    double duration_ms = 0;
};

inline LatencyStats calculate_stats(std::vector<double>& samples) {
    LatencyStats s;
    if (samples.empty())
        return s;

    std::sort(samples.begin(), samples.end());

    size_t n = samples.size();
    size_t idx_p50 = static_cast<size_t>(n * 0.50);
    size_t idx_p90 = static_cast<size_t>(n * 0.90);
    size_t idx_p99 = static_cast<size_t>(n * 0.99);
    size_t idx_p999 = static_cast<size_t>(n * 0.999);

    s.min_ns = samples.front();
    s.max_ns = samples.back();
    s.p50_ns = samples[idx_p50];
    s.p90_ns = samples[idx_p90];
    s.p99_ns = samples[idx_p99];
    s.p999_ns = samples[idx_p999];

    double sum = 0;
    for (double v : samples) {
        sum += v;
    }
    s.avg_ns = sum / n;

    double variance_sum = 0;
    for (double v : samples) {
        double diff = v - s.avg_ns;
        variance_sum += diff * diff;
    }
    s.stddev_ns = std::sqrt(variance_sum / n);

    return s;
}

void print_latency_stats(const char* name, const LatencyStats& s) {
    printf("  %s:\n", name);
    printf("    avg:    %8.1f ns  (stddev: %.1f ns)\n", s.avg_ns, s.stddev_ns);
    printf("    p50:    %8.1f ns\n", s.p50_ns);
    printf("    p90:    %8.1f ns\n", s.p90_ns);
    printf("    p99:    %8.1f ns\n", s.p99_ns);
    printf("    p99.9:  %8.1f ns\n", s.p999_ns);
    printf("    min:    %8.1f ns\n", s.min_ns);
    printf("    max:    %8.1f ns\n", s.max_ns);
}

void print_throughput_stats(const char* name, const ThroughputStats& s) {
    printf("  %s:\n", name);
    printf("    total:  %zu ops\n", s.total_ops);
    printf("    rate:   %.0f ops/s\n", s.ops_per_sec);
    printf("    time:   %.2f ms\n", s.duration_ms);
}

double measure_tuple_construct_and_storage(int iterations) {
    std::vector<double> samples;
    samples.reserve(iterations);

    using Tuple = std::tuple<int, int>;
    alignas(128) char storage[128];

    for (int i = 0; i < iterations; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        new (storage) Tuple(i, i * 2);
        auto t1 = std::chrono::high_resolution_clock::now();
        samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        reinterpret_cast<Tuple*>(storage)->~Tuple();
    }

    LatencyStats s = calculate_stats(samples);
    printf("  tuple construct + storage: avg=%.1f ns, p99=%.1f ns\n", s.avg_ns, s.p99_ns);
    return s.avg_ns;
}

double measure_ringbuffer_enqueue_only(int iterations) {
    tiny_logger::RingBuffer rb(BUFFER_SIZE);
    tiny_logger::LogEvent event;
    event.level = tiny_logger::LogLevel::Info;
    event.timestamp = 0;
    event.thread_id = 0;
    event.fmt = "test";

    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        rb.enqueue(std::move(event));
    }

    std::vector<double> samples;
    samples.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        rb.enqueue(std::move(event));
        auto t1 = std::chrono::high_resolution_clock::now();
        samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    LatencyStats s = calculate_stats(samples);
    printf("  RingBuffer::enqueue: avg=%.1f ns, p99=%.1f ns\n", s.avg_ns, s.p99_ns);
    return s.avg_ns;
}

void test_latency_with_null_printer() {
    printf("\n========== 延迟测试 (NullPrinter) ==========\n");
    printf("测量：fmt::format + RingBuffer::enqueue（主线程开销）\n\n");

    tiny_logger::register_null_printer();

    auto logger = tiny_logger::LoggerBuilder()
                      .set_buffer_size(BUFFER_SIZE)
                      .set_overflow_policy(tiny_logger::OverflowPolicy::Block)
                      .add_printer(tiny_logger::PrinterType::Null, tiny_logger::LogLevel::Debug)
                      .build();

    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        logger.info("warmup {} {}", i, i);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::vector<double> samples;
    samples.reserve(MEASURE_ITERATIONS);

    for (int i = 0; i < MEASURE_ITERATIONS; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        logger.info("msg {} {}", i, i * 2);
        auto t1 = std::chrono::high_resolution_clock::now();
        samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    logger.shutdown();

    LatencyStats s = calculate_stats(samples);
    print_latency_stats("logger.info() 端到端延迟", s);
}

void test_throughput() {
    printf("\n========== 吞吐量测试（Null Printer） ==========\n");

    tiny_logger::register_null_printer();

    printf("\n  单线程吞吐量：\n");
    {
        auto logger = tiny_logger::LoggerBuilder()
                          .set_buffer_size(1024)
                          .set_overflow_policy(tiny_logger::OverflowPolicy::Block)
                          .add_printer(tiny_logger::PrinterType::Null, tiny_logger::LogLevel::Debug)
                          .build();

        for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
            logger.info("warmup {} {}", i, i);
        }

        auto start = std::chrono::steady_clock::now();
        size_t count = 0;
        auto deadline = start + std::chrono::milliseconds(THROUGHPUT_DURATION_MS);

        while (std::chrono::steady_clock::now() < deadline) {
            logger.info("throughput test {} {}", count, count);
            ++count;
        }

        auto end = std::chrono::steady_clock::now();
        double duration_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;

        ThroughputStats ts;
        ts.total_ops = count;
        ts.duration_ms = duration_ms;
        ts.ops_per_sec = (count * 1000.0) / duration_ms;

        print_throughput_stats("NullPrinter 单线程", ts);
        logger.shutdown();
    }

    printf("\n  多线程吞吐量：\n");
    for (size_t t = 0; t < sizeof(CONCURRENT_THREADS) / sizeof(CONCURRENT_THREADS[0]); ++t) {
        int num_threads = CONCURRENT_THREADS[t];

        auto logger = tiny_logger::LoggerBuilder()
                          .set_buffer_size(1024)
                          .set_overflow_policy(tiny_logger::OverflowPolicy::Block)
                          .add_printer(tiny_logger::PrinterType::Null, tiny_logger::LogLevel::Debug)
                          .build();

        for (int i = 0; i < WARMUP_ITERATIONS * num_threads; ++i) {
            logger.info("warmup {} {}", i, i);
        }

        std::atomic<size_t> total_count{0};
        std::vector<std::thread> threads;

        auto start = std::chrono::steady_clock::now();

        for (int th = 0; th < num_threads; ++th) {
            threads.emplace_back([&logger, &total_count, deadline = start + std::chrono::milliseconds(THROUGHPUT_DURATION_MS)] {
                size_t local_count = 0;
                while (std::chrono::steady_clock::now() < deadline) {
                    logger.info("concurrent {} {}", local_count, local_count * 2);
                    ++local_count;
                }
                total_count.fetch_add(local_count, std::memory_order_relaxed);
            });
        }

        for (auto& th : threads) {
            th.join();
        }

        auto end = std::chrono::steady_clock::now();
        double duration_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;

        ThroughputStats ts;
        ts.total_ops = total_count.load();
        ts.duration_ms = duration_ms;
        ts.ops_per_sec = (ts.total_ops * 1000.0) / duration_ms;

        printf("  %d 线程: ", num_threads);
        printf("%.0f ops/s (%zu total)\n", ts.ops_per_sec, ts.total_ops);

        logger.shutdown();
    }
}

void test_ringbuffer_alone() {
    printf("\n========== RingBuffer 微基准测试 ==========\n");

    measure_tuple_construct_and_storage(MEASURE_ITERATIONS);
    printf("\n");
    measure_ringbuffer_enqueue_only(MEASURE_ITERATIONS);
}

} // anonymous namespace

int main() {
    printf("========================================\n");
    printf("  TinyLogger 性能测试\n");
    printf("========================================\n");
    printf("\n配置:\n");
    printf("  warmup:   %d iterations\n", WARMUP_ITERATIONS);
    printf("  measure:  %d iterations\n", MEASURE_ITERATIONS);
    printf("  buffer:   %zu slots\n", BUFFER_SIZE);
    printf("  throughput duration: %d ms\n", THROUGHPUT_DURATION_MS);

    test_ringbuffer_alone();
    test_latency_with_null_printer();
    test_throughput();

    printf("\n========== 环节分解 (实测) ==========\n");
    printf(R"(
logger.info() 执行流程与开销：

  [主线程 - 同步等待部分]
  1. tuple 构造         将参数打包到 128B storage 中 (placement new)
  2. format_fn/destroy_fn 设置回调函数指针
  3. RingBuffer::enqueue 无锁入队

  [后台线程 - 异步消费部分]
  4. RingBuffer::dequeue 无锁出队
  5. event.format_fn    调用回调函数，通过 std::apply + fmt::format 解包 tuple
  6. Printer::write    写入已格式化字符串 (追加时间戳、级别、线程ID等)
  7. event.destroy     调用回调函数，析构 storage 中的 tuple

关键点：
  - 步骤 1-3 在主线程测量，得到端到端延迟
  - 步骤 4-7 由后台线程执行，不阻塞主线程
  - 延迟测试使用 NullPrinter 排除 I/O 干扰
  - 吞吐量测试评估极限处理能力
  - 格式化在 Printer 线程中延迟执行，实现零拷贝日志
)");
    return 0;
}