# TinyLogger

**[English](./README.md)** | **[简体中文](./README.zh-Hans.md)** | **[繁體中文](./README.zh-Hant.md)**

C++17 high-performance asynchronous logging library based on lock-free ring buffer and multi-consumer architecture.

## Core Features

- **Asynchronous Logging** - Zero blocking for application threads, average log write latency ~130ns (p99 < 400ns)
- **Lock-free Design** - SPSC ring buffer (based on Disruptor message queue architecture), single-thread throughput exceeds 7M logs/s
- **High Concurrency** - 15M logs/s with 8 threads concurrent writing
- **RAII Resource Management** - Automatic cleanup of threads, files and other resources
- **Multiple Output Targets** - Console, File, Null Printer, supports simultaneous configuration
- **Type-safe Configuration** - Chainable Builder API with compile-time checks

## Detailed Documentation

| Document | Description |
|----------|-------------|
| [USER_GUIDE.md](docs/USER_GUIDE.md) | User guide, complete API reference, advanced usage, FAQ |
| [DEVELOPER.md](docs/DEVELOPER.md) | Developer documentation, build system, testing specifications, architecture design |

---

## Quick Start

### Install Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install libfmt-dev nlohmann-json3-dev

# Arch Linux
sudo pacman -S fmt nlohmann-json

# Fedora
sudo dnf install fmt-devel nlohmann-json-devel
```

### Build and Install

```bash
git clone <repository-url>
cd TinyLogger
mkdir build && cd build
cmake ..
make
sudo make install      # Optional, install to /usr/local
```

### Basic Usage

```cpp
#include <TinyLogger/logger_builder.h>

int main() {
    using namespace tiny_logger;

    auto logger = LoggerBuilder()
        .set_buffer_size(256)
        .set_overflow_policy(OverflowPolicy::Discard)
        .add_console_printer(LogLevel::Debug)
        .build_shared();

    logger.info("Application started");
    logger.debug("Debug info: {}", 42);
    logger.error("Error: {}", "Detailed information");

    return 0;
}
```

Compile:

```bash
g++ -std=c++17 -I/path/to/TinyLogger/include -o myapp myapp.cpp \
    -L/path/to/TinyLogger/build -lTinyLogger -lfmt
```

**Note:** `LoggerRef` supports copy and dot operator (`.info()`, `.debug()`, etc.) for cleaner API.

---

## Core Concepts

### Architecture Overview

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Logger    │────▶│ RingBuffer  │────▶│ Distributor │────▶│  Printers   │
│  (API)      │     │ (lock-free) │     │ (thread)    │     │ Console/File│
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
```

### Core Components

| Component | Description |
|-----------|-------------|
| **Logger** | User API, encapsulates LogEvent and writes to RingBuffer |
| **RingBuffer** | SPSC lock-free ring buffer, Disruptor-style |
| **Distributor** | Dedicated thread, consumes log events and dispatches to Printers |
| **Printers** | Output handlers: Console, File, Null |

### Log Levels

| Level | Description | Recommended Scenario |
|-------|-------------|----------------------|
| `Debug` | Debug information | Detailed tracking during development |
| `Info` | General information | Normal operation status logging |
| `Error` | Error information | Exceptions that don't affect execution |
| `Fatal` | Fatal error | Program cannot continue |

Level filtering rule: A log is only recorded when its level ≥ the `min_level` configured for the Printer.

---

## Performance Benchmark

TinyLogger uses a lock-free ring buffer and asynchronous dispatch design to minimize the impact of logging on the main business thread. The following are measured data points for reference only:

### Configuration

- Buffer Size: 256 slots
- Payload: 128B storage per event
- Iterations: 50,000 measurements (after 10,000 warmup)
- Hardware: 2-core CPU, Linux 5.15.0-164-generic

### Latency Test

Measures the time overhead from calling `logger.info()` to function return (i.e., main thread wait portion). Tests use `NullPrinter` to eliminate disk I/O interference.

| Metric | Time (ns) | Description |
|--------|-----------|-------------|
| Average Latency | 138.7 ns | Ultra-fast response, main thread overhead below microseconds |
| Median (p50) | 112.0 ns | Core overhead for the vast majority of calls |
| p99 Percentile | 345.0 ns | Even under high concurrency, tail latency remains controlled |
| Minimum Latency | 56.0 ns | Fastest write under ideal conditions |

### Throughput Test

Evaluates the system's extreme processing capability under sustained high load. Also uses `NullPrinter` to eliminate disk I/O interference.

| Concurrent Threads | Throughput (ops/s) |
|--------------------|--------------------|
| 1 thread | 7.08 M |
| 2 threads | 7.31 M |
| 4 threads | 14.21 M |
| 8 threads | 15.26 M |

### Key Path Overhead Breakdown

| Step | Description | Time |
|------|-------------|------|
| 1. tuple construction | Pack parameters into 128B storage | ~71 ns |
| 2. RingBuffer::enqueue | Lock-free enqueue | ~35 ns |
| 3. RingBuffer::dequeue | Lock-free dequeue (background thread) | - |
| 4. event.format_fn | Call callback function for formatting | - |
| 5. Printer::write | Write to output (background thread) | - |

**Key Design:**

- Main thread deferred formatting: `logger.log()` only performs tuple construction of parameters on the main thread (using placement new to write to pre-allocated space) and enqueues. Time-consuming `fmt::format()` string concatenation and I/O writing are executed asynchronously by the background thread.
- Lock-free RingBuffer: Each application thread has its own independent SPSC queue for `RingBuffer::enqueue()`, achieving contention-free enqueue.
- RAII Resource Management: Ensures all asynchronous tasks are properly flushed and reclaimed before system shutdown.

**Performance Recommendations:**

- Use `OverflowPolicy::Discard` in production to avoid blocking latency spikes
- When batch logging, the background thread can fully digest, resulting in more stable latency

### Reproduction

You can verify these data by running the built-in test suite through the build system:

```bash
cd build
make run_tests  # Includes unit tests and benchmark tests
```

---

## Build and Install

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `TINYLOGGER_BUILD_TESTS` | ON | Build test suite |
| `TINYLOGGER_BUILD_EXAMPLES` | ON | Build example programs |
| `CMAKE_BUILD_TYPE` | - | Build type (Release/Debug) |
| `CMAKE_INSTALL_PREFIX` | /usr/local | Installation path |

### Build Commands

```bash
# Full build
mkdir build && cd build && cmake .. && make

# Release build (recommended)
mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make

# Build library only
cmake .. -DTINYLOGGER_BUILD_TESTS=OFF -DTINYLOGGER_BUILD_EXAMPLES=OFF

# Build tests only
cmake .. -DTINYLOGGER_BUILD_EXAMPLES=OFF

# Build examples only
cmake .. -DTINYLOGGER_BUILD_TESTS=OFF

# Custom installation path
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/tinylogger && make install
```

### Cleanup

```bash
make clean              # Clean build artifacts
make clean-all          # Clean build + test temp files + example outputs
```

---

## Configuration

### Builder Chain Configuration (Recommended)

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(512)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)        // Console output
    .add_file_printer("app.log", LogLevel::Info)  // File output
    .build_shared();
```

### Configuration Options

| Method | Parameters | Default | Description |
|--------|------------|---------|-------------|
| `set_buffer_size(size)` | size_t | 256 | Ring buffer size (must be power of 2) |
| `set_overflow_policy(policy)` | OverflowPolicy | Discard | Overflow policy |
| `add_console_printer(level)` | LogLevel | Info | Add console Printer |
| `add_file_printer(path, level)` | string, LogLevel | Debug | Add file Printer |

### Overflow Policies

| Policy | Description | Notes |
|--------|-------------|-------|
| `Discard` | Discard new logs (default) | Better performance, suitable for production |
| `Block` | Block and wait for space | May cause 3ms+ latency spikes, use with caution |

### Default Configuration

```cpp
auto logger = tiny_logger::create_default_logger();
```

Uses default configuration: Console Printer + Info level + Discard overflow policy.

**Note:** Logger is non-copyable. Use `build_shared()` to create a `std::unique_ptr<Logger>`.

---

## API Quick Reference

### Logging

```cpp
logger.debug(fmt::format_string<T...>, T&&... args);
logger.info(fmt::format_string<T...>, T&&... args);
logger.error(fmt::format_string<T...>, T&&... args);
logger.fatal(fmt::format_string<T...>, T&&... args);
```

### Examples

```cpp
logger.info("User {} logged in", username);
logger.debug("Value: {:.2f}", 3.14159);
logger.error("Error code: {:#x}", 0xDEAD);
logger.info("Multiple values: {}, {}, {}", a, b, c);
```

Log parameters should be copyable small POD types (integers, floats, C strings, etc.) with total size not exceeding 128 bytes.

---

## Using in Other Projects

After installation, use in CMakeLists.txt:

```cmake
find_package(TinyLogger REQUIRED)
target_link_libraries(your_target TinyLogger::tinylogger)
```

---

## Project Structure

```
TinyLogger/
├── include/TinyLogger/      # Header files
│   ├── logger.h
│   ├── logger_builder.h
│   ├── logger_factory.h     # Factory functions
│   ├── ring_buffer.h
│   ├── types.h
│   └── ...
├── src/                    # Implementation files
│   ├── logger.cpp
│   ├── logger_factory.cpp   # Factory functions implementation
│   ├── ring_buffer.cpp
│   ├── distributor.cpp
│   └── ...
├── test/                   # Test suite
│   ├── test_ring_buffer.cpp
│   ├── test_printer.cpp
│   ├── test_distributor.cpp
│   └── test_logger.cpp
├── examples/               # Example programs
│   ├── example.cpp
│   └── speed_test.cpp
├── docs/                   # Detailed documentation
│   ├── USER_GUIDE.md
│   └── DEVELOPER.md
└── CMakeLists.txt
```

---

## FAQ

### Q: Logs not being written to file?

Check:
1. Does the File Printer's `file_path` have write permissions?
2. Is `min_level` set correctly?
3. Have you called `shutdown()` or waited for automatic flush?

### Q: How to choose overflow policy?

- **Discard** (default): Recommended for production, better performance
- **Block**: Use when log loss cannot be tolerated, but may cause latency spikes

### Q: Cannot find fmt library at compile time?

Ensure fmt library is installed and specify the path at compile time:

```bash
g++ ... -L/usr/lib -lfmt
```

Or use CMake's auto-find.