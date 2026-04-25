# TinyLogger

**[English](./README.md)** | **[简体中文](./README.zh-Hans.md)** | **[繁體中文](./README.zh-Hant.md)**

一款轻量的 C++17 高性能异步日志库，基于无锁环形缓冲区和多消费者架构。

## 核心特性

- **异步日志** - 应用线程零阻塞，日志写入平均延迟约 130ns（p99 < 400ns）
- **无锁设计** - SPSC 环形缓冲区（基于 Disruptor 消息队列架构），单线程吞吐量超 700 万 logs/s
- **高并发** - 8 线程并发写入可达 1500 万 logs/s
- **RAII 资源管理** - 线程、文件等资源自动清理
- **多输出目标** - Console、File、Null Printer，支持同时配置多个
- **类型安全配置** - 链式 Builder API，编译期检查

## 详细文档

| 文档 | 说明 |
|------|------|
| [USER_GUIDE.md](docs/USER_GUIDE.md) | 用户指南，包含完整 API 参考、高级用法、FAQ |
| [DEVELOPER.md](docs/DEVELOPER.md) | 开发者文档，包含构建系统、测试规范、架构设计 |

---

## 快速开始

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install libfmt-dev nlohmann-json3-dev

# Arch Linux
sudo pacman -S fmt nlohmann-json

# Fedora
sudo dnf install fmt-devel nlohmann-json-devel
```

### 构建与安装

```bash
git clone <repository-url>
cd TinyLogger
mkdir build && cd build
cmake ..
make
sudo make install      # 可选，安装到 /usr/local
```

### 基本使用

```cpp
#include <TinyLogger/logger_builder.h>

int main() {
    using namespace tiny_logger;

    auto logger = LoggerBuilder()
        .set_buffer_size(256)
        .set_overflow_policy(OverflowPolicy::Discard)
        .add_console_printer(LogLevel::Debug)
        .build();

    logger.info("应用程序启动");
    logger.debug("调试信息：{}", 42);
    logger.error("错误：{}", "详细信息");

    return 0;
}
```

编译：

```bash
g++ -std=c++17 -I/path/to/TinyLogger/include -o myapp myapp.cpp \
    -L/path/to/TinyLogger/build -lTinyLogger -lfmt
```

**注意：** `LoggerRef` 支持拷贝和点操作符（`.info()`、`.debug()` 等），API 更美观。

---

## 核心概念

### 简要架构图

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Logger    │────▶│ RingBuffer  │────▶│ Distributor │────▶│  Printers   │
│  (API)      │     │ (lock-free) │     │ (thread)    │     │ Console/File│
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
```

### 核心组件

| 组件 | 说明 |
|------|------|
| **Logger** | 用户 API，封装 LogEvent 并写入 RingBuffer |
| **RingBuffer** | SPSC 无锁环形缓冲区，Disruptor 风格 |
| **Distributor** | 专用线程，消费日志事件并分发给 Printers |
| **Printers** | 输出处理器：Console、File、Null |

### 日志级别

| 级别 | 说明 | 建议场景 |
|------|------|----------|
| `Debug` | 调试信息 | 开发阶段详细追踪 |
| `Info` | 一般信息 | 正常运行状态记录 |
| `Error` | 错误信息 | 异常但不影响运行 |
| `Fatal` | 致命错误 | 程序无法继续 |

级别过滤规则：只有当日志级别 ≥ Printer 配置的 `min_level` 时才会被记录。

---

## 性能基准 (Performance Benchmark)

TinyLogger 采用无锁环形缓冲区（Lock-free RingBuffer）与异步分发设计，旨在将日志记录对业务主线程的影响降至最低。以下是一组实测数据，仅供参考：

### 配置

- Buffer Size: 256 slots
- Payload: 128B storage per event
- Iterations: 50,000 measurements (after 10,000 warmup)
- Hardware: 2 核 CPU，Linux 5.15.0-164-generic

### 延迟测试 (Latency)

测量从调用 `logger.info()` 到函数返回（即主线程等待部分）的时间开销。测试使用 `NullPrinter` 以排除磁盘 I/O 的干扰。

| 指标           | 耗时 (ns) | 说明                         |
| ------------------ | ------------- | -------------------------------- |
| 平均延迟 (Avg) | 138.7 ns  | 极速响应，主线程开销微秒级以下   |
| 中位数 (p50)   | 112.0 ns  | 绝大多数调用的核心开销           |
| p99 分位       | 345.0 ns  | 即使在高并发下，长尾延迟依然受控 |
| 最小延迟 (Min) | 56.0 ns       | 理想情况下的最快写入             |

### 吞吐量测试 (Throughput)

评估系统在持续高负载下的极限处理能力。同样使用 `NullPrinter` 以排除磁盘 I/O 的干扰。

| 并发线程数 | 吞吐量 (ops/s) |
|-----------|----------------|
| 1 线程 | 7.08 M |
| 2 线程 | 7.31M |
| 4 线程 | 14.21 M |
| 8 线程 | 15.26 M |

### 关键路径开销分解

| 环节 | 说明 | 耗时 |
|------|------|------|
| 1. tuple 构造 | 将参数打包到 128B storage | ~71 ns |
| 2. RingBuffer::enqueue | 无锁入队 | ~35 ns |
| 3. RingBuffer::dequeue | 无锁出队（后台线程） | - |
| 4. event.format_fn | 调用回调函数格式化 | - |
| 5. Printer::write | 写入输出（后台线程） | - |

**关键设计：**
- 主线程延迟格式化: `logger.log()` 在主线程仅执行参数的 tuple 构造（使用 placement new 写入预分配空间）并入队。耗时的 `fmt::format()` 字符串拼接和 I/O 写入完全由后台线程异步执行。
- 无锁 RingBuffer: `RingBuffer::enqueue()` 每个应用线程独立拥有一个 SPSC 队列，实现无竞争入队。
- RAII 资源管理: 确保所有异步任务在系统关闭（shutdown）前被正确刷新和回收。

**性能建议：**
- 生产环境优先使用 `OverflowPolicy::Discard`，避免阻塞延迟尖峰
- 批量日志写入时，后台线程可充分消化，延迟更稳定

### 复现方式

你可以通过构建系统运行自带的测试套件来验证这些数据：

```bash
cd build
make run_tests  # 包含单元测试与基准测试
```

---

## 构建与安装

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `TINYLOGGER_BUILD_TESTS` | ON | 构建测试套件 |
| `TINYLOGGER_BUILD_EXAMPLES` | ON | 构建示例程序 |
| `CMAKE_BUILD_TYPE` | - | 构建类型（Release/Debug） |
| `CMAKE_INSTALL_PREFIX` | /usr/local | 安装路径 |

### 构建命令

```bash
# 完整构建
mkdir build && cd build && cmake .. && make

# Release 构建（推荐）
mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make

# 仅构建库
cmake .. -DTINYLOGGER_BUILD_TESTS=OFF -DTINYLOGGER_BUILD_EXAMPLES=OFF

# 仅构建测试
cmake .. -DTINYLOGGER_BUILD_EXAMPLES=OFF

# 仅构建示例
cmake .. -DTINYLOGGER_BUILD_TESTS=OFF

# 自定义安装路径
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/tinylogger && make install
```

### 清理

```bash
make clean              # 清理构建产物
make clean-all        # 清理构建 + 测试临时文件 + 示例产物
```

---

## 配置方式

### Builder 链式配置（推荐）

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(512)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)        // 控制台输出
    .add_file_printer("app.log", LogLevel::Info)  // 文件输出
    .build();
```

### 配置项说明

| 方法 | 参数 | 默认值 | 说明 |
|------|------|--------|------|
| `set_buffer_size(size)` | size_t | 256 | 环形缓冲区大小（必须是 2 的幂次） |
| `set_overflow_policy(policy)` | OverflowPolicy | Discard | 溢出策略 |
| `add_console_printer(level)` | LogLevel | Info | 添加控制台 Printer |
| `add_file_printer(path, level)` | string, LogLevel | Debug | 添加文件 Printer |

### 溢出策略

| 策略 | 说明 | 注意事项 |
|------|------|----------|
| `Discard` | 丢弃新日志（默认） | 性能更好，适合生产环境 |
| `Block` | 阻塞等待空间 | 可能导致 3ms+ 延迟尖峰，慎用 |

### 默认配置

```cpp
auto logger = tiny_logger::create_default_logger();
```

使用默认配置：Console Printer + Info 级别 + Discard 溢出策略。

**注意：** Logger 不可拷贝，请使用 `build()` 创建 `LoggerRef`（基于 shared_ptr）。

---

## API 快速参考

### 日志记录

```cpp
logger.debug(fmt::format_string<T...>, T&&... args);
logger.info(fmt::format_string<T...>, T&&... args);
logger.error(fmt::format_string<T...>, T&&... args);
logger.fatal(fmt::format_string<T...>, T&&... args);
```

### 示例

```cpp
logger.info("用户 {} 登录", username);
logger.debug("数值：{:.2f}", 3.14159);
logger.error("错误码：{:#x}", 0xDEAD);
logger.info("多个值：{}, {}, {}", a, b, c);
```

日志参数应为可拷贝的小型 POD 类型（整数、浮点、C 字符串等），且总大小不超过 128 字节。

---

## 在其他项目中使用

安装后，在 CMakeLists.txt 中使用：

```cmake
find_package(TinyLogger REQUIRED)
target_link_libraries(your_target TinyLogger::tinylogger)
```

---

## 项目结构

```
TinyLogger/
├── include/TinyLogger/      # 头文件
│   ├── logger.h
│   ├── logger_builder.h
│   ├── logger_factory.h     # 工厂函数
│   ├── ring_buffer.h
│   ├── types.h
│   └── ...
├── src/                    # 实现文件
│   ├── logger.cpp
│   ├── logger_factory.cpp   # 工厂函数实现
│   ├── ring_buffer.cpp
│   ├── distributor.cpp
│   └── ...
├── test/                   # 测试套件
│   ├── test_ring_buffer.cpp
│   ├── test_printer.cpp
│   ├── test_distributor.cpp
│   └── test_logger.cpp
├── examples/               # 示例程序
│   ├── example.cpp
│   └── speed_test.cpp
├── docs/                   # 详细文档
│   ├── USER_GUIDE.md
│   └── DEVELOPER.md
└── CMakeLists.txt
```

---

## 常见问题

### Q: 日志没有写入文件？

检查：
1. File Printer 的 `file_path` 是否有写权限
2. `min_level` 设置是否正确
3. 是否调用了 `shutdown()` 或等待自动 flush

### Q: 如何选择溢出策略？

- **Discard**（默认）：生产环境推荐，性能更好
- **Block**：需要保证不丢日志时使用，但可能引发延迟尖峰

### Q: 编译时找不到 fmt 库？

确保已安装 fmt 库，并在编译时指定路径：

```bash
g++ ... -L/usr/lib -lfmt
```

或使用 CMake 自动查找。
