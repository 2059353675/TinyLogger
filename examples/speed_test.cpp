/**
 * TinyLogger 速度测试示例 - 各环节耗时精准统计
 *
 * 测试方法：
 *   1. 单线程 warmed-up 测试，避免 buffer 溢出
 *   2. 分别测量 fmt::format 和 logger.info() 返回时间
 *   3. 对比 NullPrinter 和 ConsolePrinter 估算 I/O 开销
 *
 * 编译方式（Linux）：
 *   g++ -std=c++17 -I../include -O2 -o speed_test speed_test.cpp -L../build -lTinyLogger -lfmt -lpthread
 *
 * 或者使用 CMake 构建（在项目根目录执行）：
 *   mkdir build && cd build
 *   cmake .. && make
 *   ./speed_test
 */

#include <TinyLogger/logger.h>
#include <TinyLogger/printer_console.h>
#include <TinyLogger/printer_null.h>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <thread>
#include <vector>

struct TimingStats {
    double avg_ns = 0;
    double min_ns = 0;
    double max_ns = 0;
    double p50_ns = 0;
    double p99_ns = 0;
};

static std::vector<double> g_samples;

void reset_samples() {
    g_samples.clear();
}
void add_sample(double ns) {
    g_samples.push_back(ns);
}

TimingStats analyze() {
    TimingStats s;
    if (g_samples.empty())
        return s;

    std::sort(g_samples.begin(), g_samples.end());
    s.min_ns = g_samples.front();
    s.max_ns = g_samples.back();
    s.p50_ns = g_samples[g_samples.size() / 2];
    s.p99_ns = g_samples[static_cast<size_t>(g_samples.size() * 0.99)];

    double sum = 0;
    for (double v : g_samples)
        sum += v;
    s.avg_ns = sum / g_samples.size();
    return s;
}

struct TestResult {
    TimingStats info_return;
    TimingStats fmt_standalone;
    size_t dropped = 0;
};

TestResult run_test(TinyLogger::Logger& logger, int warmup, int measure) {
    reset_samples();
    for (int i = 0; i < warmup; ++i) {
        logger.info("warmup {}", i);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    reset_samples();
    for (int i = 0; i < measure; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        logger.info("msg {}", i);
        auto t1 = std::chrono::high_resolution_clock::now();
        add_sample(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    auto info_stats = analyze();

    reset_samples();
    char buf[256];
    for (int i = 0; i < measure; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        fmt::format_to_n(buf, sizeof(buf) - 1, "msg {}", i);
        auto t1 = std::chrono::high_resolution_clock::now();
        add_sample(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    auto fmt_stats = analyze();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    logger.shutdown();

    return {info_stats, fmt_stats, logger.dropped_count()};
}

void print(const char* name, const TimingStats& s) {
    printf("  %s:\n", name);
    printf("    avg:  %8.1f ns\n", s.avg_ns);
    printf("    p50:  %8.1f ns\n", s.p50_ns);
    printf("    p99:  %8.1f ns\n", s.p99_ns);
    printf("    min:  %8.1f ns\n", s.min_ns);
    printf("    max:  %8.1f ns\n", s.max_ns);
}

int main() {
    printf("===== TinyLogger 各环节耗时精准统计 =====\n");
    printf("测试：单线程 warmed-up，无 buffer 溢出\n\n");

    const int WARMUP = 500;
    const int MEASURE = 5000;

    printf("\n【测试1】ConsolePrinter (含 I/O)\n");
    {
        TinyLogger::Logger logger;

        TinyLogger::PrinterConfig console_config;
        console_config.type = TinyLogger::PrinterType::Console;
        console_config.min_level = TinyLogger::LogLevel::Debug;

        TinyLogger::LoggerConfig config;
        config.overflow_policy = TinyLogger::OverflowPolicy::Block;
        config.printers.push_back(console_config);

        if (logger.init(config) != TinyLogger::ErrorCode::None) {
            printf("  初始化失败\n");
            return 1;
        } else {
            auto r = run_test(logger, WARMUP, MEASURE);
            printf("  丢弃: %zu\n", r.dropped);
            print("logger.info() 返回", r.info_return);
            print("fmt::format 单独", r.fmt_standalone);
        }
    }

    printf("【测试2】NullPrinter (无 I/O)\n");
    {
        TinyLogger::register_null_printer();

        TinyLogger::PrinterConfig null_config;
        null_config.type = TinyLogger::PrinterType::Null;
        null_config.min_level = TinyLogger::LogLevel::Debug;

        TinyLogger::Logger logger;
        TinyLogger::LoggerConfig config;
        config.overflow_policy = TinyLogger::OverflowPolicy::Block;
        config.printers.push_back(null_config);

        if (logger.init(config) != TinyLogger::ErrorCode::None) {
            printf("  初始化失败\n");
            return 1;
        } else {
            auto r = run_test(logger, WARMUP, MEASURE);
            printf("  丢弃: %zu\n", r.dropped);
            print("logger.info() 返回", r.info_return);
            print("fmt::format 单独", r.fmt_standalone);
        }
    }

    printf("\n===== 环节分解 =====\n");
    printf(R"(
调用 logger.info("msg {} {}", a, b) 时的执行流程：

  [主线程]
  1. fmt::format    格式化日志消息 (~50-200 ns)
  2. RingBuffer::enqueue  无锁入队 (~50-100 ns)

  [后台消费线程 - 异步]
  3. Distributor::dequeue  无锁出队
  4. Printer::write       再次格式化 + 拼接时间戳
  5. fwrite               写入 stdout

关键点：
  - 主线程只执行步骤 1-2，步骤 3-5 由后台线程异步执行
  - logger.info() 返回时间 = 步骤 1-2 的主线程开销
  - fmt::format 单独测量 = 步骤 1 的纯格式化开销
  - Printer::write + fwrite 由后台线程执行，不影响主线程延迟
)");

    return 0;
}