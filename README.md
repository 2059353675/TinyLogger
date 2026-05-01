# TinyLogger

**[English](./README.md)** | **[简体中文](./README.zh-Hans.md)** | **[繁體中文](./README.zh-Hant.md)**

C++17 high-performance asynchronous logging library based on lock-free ring buffer and multi-consumer architecture.

## Core Features

- **Asynchronous Logging** - Zero blocking for application threads, average log write latency ~130ns (p99 < 400ns)
- **Lock-free Design** - SPSC ring buffer, single-thread throughput exceeds 7M logs/s
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
cmake ..                                                  # Linux/Unix
cmake .. -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"  # Windows
make
sudo make install      # Optional, install to /usr/local
```

### Basic Usage

```cpp
#include <tiny_logger/logger_builder.h>

int main() {
    using namespace tiny_logger;

    auto logger = LoggerBuilder()
        .set_buffer_size(256)
        .set_overflow_policy(OverflowPolicy::Discard)
        .add_console_printer(LogLevel::Debug)
        .build();

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
| `Warn` | Warning information | Potential issues that don't affect execution |
| `Error` | Error information | Exceptions that don't affect execution |
| `Fatal` | Fatal error | Program cannot continue |

Level filtering rule: A log is only recorded when its level ≥ the `min_level` configured for the Printer.

---

## Performance Benchmark

TinyLogger uses a high-precision timing method based on TSC (rdtsc), along with CPU pinning and baseline comparison to ensure stable and reliable test results.

### Configuration

Compilation flags: `-O3 -march=native`
CPU frequency: approximately 2.50 GHz (≈ 2.50 cycles/ns)
Single-core binding (taskset / pthread_setaffinity_np)
`NullPrinter` used to eliminate I/O interference
Batch measurement (batch=64) to reduce timing noise

### Baseline

| Test Item         | Average (cycles) | p50   | p99   | Description                          |
| ----------------- | ---------------- | ----- | ----- | ------------------------------------ |
| Empty function call | 16.1             | 15.4  | 23.8  | Measures function call and loop overhead |

### Latency Test

The latency test focuses on the main thread path delay, i.e., the overhead before `logger.info()` returns.

| Test Item                   | Average (cycles) | p50    | p99    | Description                          |
| --------------------------- | ---------------- | ------ | ------ | ------------------------------------ |
| `fmt::format`               | 1380.7           | 1319.0 | 2133.9 | Synchronous string formatting        |
| `RingBuffer::enqueue`       | 62.7             | 56.6   | 85.9   | Lock-free enqueue (SPSC)             |
| `logger.info()`             | 896.5            | 396.8  | 738.5  | Complete main thread path            |

### Throughput Test

The throughput test evaluates the system's peak processing capability under sustained high load, focusing on multi-threaded concurrent logging scenarios.

| Number of Concurrent Threads | Throughput (ops/s) |
|------------------------------|--------------------|
| 1 thread                     | 7.08 M             |
| 2 threads                    | 7.31 M             |
| 4 threads                    | 14.21 M            |
| 8 threads                    | 15.26 M            |

### Critical Path Overhead Breakdown

| Test Item                   | Average (cycles) | p50    | p99    | Description                          |
| --------------------------- | ---------------- | ------ | ------ | ------------------------------------ |
| `fmt::format()`             | 1380.7           | 1319.0 | 2133.9 | Synchronous string formatting        |
| `RingBuffer::enqueue()`     | 62.7             | 56.6   | 85.9   | Lock-free enqueue (SPSC)             |
| `logger->log()`             | 896.5            | 396.8  | 738.5  | Complete main thread path            |

Converted to time (2.5 cycles/ns):

- `fmt::format` ≈ 552 ns
- `enqueue` ≈ 25 ns
- `logger.info()`:
    - p50 ≈ 160 ns
    - p99 ≈ 300 ns

**Key Design Points:**
- **Deferred formatting on main thread:** `logger->log()` performs only tuple construction (using placement new into pre-allocated space) and enqueue on the main thread. The expensive `fmt::format()` string concatenation and I/O writes are handled asynchronously by a background thread.
- **Lock-free RingBuffer:** `RingBuffer::enqueue()` — each application thread has its own SPSC queue, achieving contention-free enqueue.
- **RAII resource management:** Ensures all asynchronous tasks are properly flushed and reclaimed before system shutdown.

**Performance Recommendations:**
- Prefer `OverflowPolicy::Discard` in production environments to avoid blocking latency spikes.
- For batch log writes, the background thread can fully digest them, resulting in more stable latency.

### Reproduction

```bash
mkdir build && cd build && cmake .. -DTINYLOGGER_BUILD_EXAMPLES=ON && make
./benchmark
```

---

## Build and Install

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `TINYLOGGER_BUILD_TESTS` | ON | Build test suite |
| `TINYLOGGER_BUILD_EXAMPLES` | ON | Build example programs |
| `USE_SYSTEM_FMT` | OFF | Use system-installed fmt instead of FetchContent |
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
    .build();
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

**Note:** Logger is non-copyable. Use `build()` to create a `LoggerRef` (shared_ptr based).

---

## API Quick Reference

### Logging

```cpp
logger.debug(fmt::format_string<T...>, T&&... args);
logger.info(fmt::format_string<T...>, T&&... args);
logger.warn(fmt::format_string<T...>, T&&... args);
logger.error(fmt::format_string<T...>, T&&... args);
logger.fatal(fmt::format_string<T...>, T&&... args);
```

### Examples

```cpp
logger.info("User {} logged in", username);
logger.warn("Warning: memory usage at {}%", 85);
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
├── include/tiny_logger/      # Header files
│   ├── logger.h
│   ├── logger_builder.h
│   ├── logger_factory.h     # Factory functions
│   ├── logger_error.h
│   ├── ring_buffer.h
│   ├── distributor.h
│   ├── printer.h
│   ├── printer/             # Printer subdirectory
│   │   ├── base.h
│   │   ├── console.h
│   │   ├── file.h
│   │   └── null.h
│   ├── types.h
│   └── ...
├── src/                    # Implementation files
│   ├── logger.cpp
│   ├── logger_builder.cpp
│   ├── logger_factory.cpp
│   ├── ring_buffer.cpp
│   ├── distributor.cpp
│   ├── queue_registry.cpp
│   ├── console.cpp         # Console Printer
│   ├── file.cpp            # File Printer
│   ├── null.cpp            # Null Printer
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