#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fmt/format.h>
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <tiny_logger/logger_builder.h>
#include <tiny_logger/printer/null.h>
#include <tiny_logger/ring_buffer.h>
#include <vector>
#include <x86intrin.h>

struct LatencyStats {
    double avg_ns;
    double p50_ns;
    double p99_ns;
};

double percentile(const std::vector<double>& sorted, double p) {
    double pos = p * (sorted.size() - 1);
    size_t idx = static_cast<size_t>(pos);
    double frac = pos - idx;

    if (idx + 1 < sorted.size()) {
        return sorted[idx] * (1 - frac) + sorted[idx + 1] * frac;
    } else {
        return sorted[idx];
    }
}

LatencyStats calculate_stats(const std::vector<double>& samples) {
    if (samples.empty())
        return {0, 0};

    // avg（基于原始数据）
    double sum = 0;
    for (double v : samples)
        sum += v;
    double avg = sum / samples.size();

    // 排序用于分位数
    std::vector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());

    size_t n = sorted.size();

    double p50 = percentile(sorted, 0.50);
    double p99 = percentile(sorted, 0.99);

    return {avg, p50, p99};
}

inline uint64_t rdtsc() {
    unsigned aux;
    return __rdtscp(&aux); // serialize + read
}

void pin_thread(int cpu_id) {
#ifdef _WIN32
    (void)cpu_id;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
    }
#endif
}

double calibrate_tsc_ghz() {
    using namespace std::chrono;

    auto t0 = steady_clock::now();
    uint64_t c0 = rdtsc();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto t1 = steady_clock::now();
    uint64_t c1 = rdtsc();

    double ns = duration_cast<nanoseconds>(t1 - t0).count();
    return (c1 - c0) / ns; // cycles per ns
}

template <typename F>
void measure_cycles_per_op(F&& fn, int iterations, int batch) {
    std::vector<double> samples;
    samples.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        uint64_t t0 = rdtsc();

        for (int j = 0; j < batch; ++j) {
            fn();
        }

        uint64_t t1 = rdtsc();
        samples.push_back(double(t1 - t0) / batch);
    }

    LatencyStats s = calculate_stats(samples);
    printf("    avg: %.1f cycles  p50: %.1f cycles  p99: %.1f cycles\n", s.avg_ns, s.p50_ns, s.p99_ns);
}

inline void noop(int, int) {
    asm volatile("" ::: "memory");
}

void bench_fmt_only(int iterations, int batch) {
    fmt::memory_buffer buf;

    auto fn = [&]() {
        buf.clear();
        fmt::format_to(std::back_inserter(buf), "msg {} {}", 42, 84);
    };

    printf("  fmt::format:\n");
    measure_cycles_per_op(fn, iterations, batch);
}

void bench_enqueue_only(int iterations, int batch) {
    tiny_logger::RingBuffer rb(256, tiny_logger::OverflowPolicy::Discard);

    auto fn = [&]() {
        tiny_logger::LogEvent e;
        e.level = tiny_logger::LogLevel::Info;
        e.timestamp = 0;
        e.thread_id = 0;
        e.fmt = "test";
        rb.enqueue(std::move(e));
    };

    printf("  RingBuffer enqueue:\n");
    measure_cycles_per_op(fn, iterations, batch);
}

void bench_logger(int iterations, int batch) {
    tiny_logger::register_null_printer();

    auto logger = tiny_logger::LoggerBuilder()
                      .set_buffer_size(1024)
                      .set_overflow_policy(tiny_logger::OverflowPolicy::Discard)
                      .add_printer(tiny_logger::PrinterType::Null, tiny_logger::LogLevel::Debug)
                      .build();

    auto fn = [&]() { logger.info("msg {} {}", 42, 84); };

    printf("  logger.info():\n");
    measure_cycles_per_op(fn, iterations, batch);
}

int main() {
    pin_thread(0); // 固定在 CPU0

    printf("Calibrating TSC...\n");
    double ghz = calibrate_tsc_ghz();
    printf("TSC: %.2f cycles/ns\n\n", ghz);

    constexpr int ITER = 20000;
    constexpr int BATCH = 64;

    printf("========== Baseline ==========\n");
    measure_cycles_per_op([&]() { noop(1, 2); }, ITER, BATCH);

    printf("\n========== fmt ==========\n");
    bench_fmt_only(ITER, BATCH);

    printf("\n========== enqueue ==========\n");
    bench_enqueue_only(ITER, BATCH);

    printf("\n========== logger ==========\n");
    bench_logger(ITER, BATCH);

    printf("\n(All results in cycles; convert via cycles/ns)\n");
}
