# TinyLogger User Guide

**[English](./USER_GUIDE.md)** | **[简体中文](./USER_GUIDE.zh-Hans.md)** | **[繁體中文](./USER_GUIDE.zh-Hant.md)**

> For users: Quick introduction to using the TinyLogger library

## Table of Contents

- [Quick Start](#quick-start)
    - [Installing Dependencies](#installing-dependencies)
    - [Building and Installing](#building-and-installing)
    - [Minimal Demo](#minimal-demo)
- [Configuration](#configuration)
    - [Configuration Options](#configuration-options)
    - [Console Printer Configuration](#console-printer-configuration)
    - [File Printer Configuration](#file-printer-configuration)
- [API Reference](#api-reference)
    - [Builder API](#builder-api)
    - [Log Levels](#log-levels)
    - [Formatted Output](#formatted-output)
- [Advanced Usage](#advanced-usage)
    - [Multiple Printer Configuration](#multiple-printer-configuration)
    - [Log File Rotation](#log-file-rotation)
- [Using in Other Projects](#using-in-other-projects)
- [Frequently Asked Questions](#frequently-asked-questions)

---

## Quick Start

### Installing Dependencies

TinyLogger requires the following dependencies:

- **C++17 compatible compiler** (GCC 9+, Clang 10+, MSVC 2019+)
- **CMake build tool**: at least 3.14
- **fmt library**: high-performance formatting output

#### Ubuntu/Debian

```bash
sudo apt-get install libfmt-dev
```

#### Arch Linux

```bash
sudo pacman -S fmt
```

#### Fedora

```bash
sudo dnf install fmt-devel
```

### Building and Installing

```bash
# Clone the project
git clone https://github.com/2059353675/TinyLogger.git
cd TinyLogger

# Build
mkdir build && cd build
cmake ..
cmake --build .

# Install (optional)
sudo cmake --install .
```

Build artifacts:

- `outputs/libTinyLogger.a` - static library
- `outputs/example` - example program
- `outputs/test_*` - test executables

### Minimal Demo

It is recommended to use `LoggerBuilder` (fluent configuration, type-safe):

```cpp
#include <tiny_logger/logger_builder.hpp>

int main() {
    using namespace tiny_logger;

    auto logger = LoggerBuilder()
        .set_buffer_size(256)
        .set_overflow_policy(OverflowPolicy::Discard)
        .add_console_printer(LogLevel::Debug)
        .build();

    logger.info("Application started");
    logger.debug("Debug info: {}", 42);
    logger.error("Error: {}", "details");

    return 0;
}
```

Or use default configuration (discard only, output Info level and above):

```cpp
#include <tiny_logger/logger_builder.hpp>

int main() {
    auto logger = tiny_logger::create_default_logger();
    logger.info("Application started");
    return 0;
}
```

Then compile your program:

```bash
g++ -std=c++17 -I /path/to/TinyLogger/include -o myapp myapp.cpp \
    -L /path/to/TinyLogger/build -lTinyLogger -lfmt
```

---

## Configuration

Use `LoggerBuilder` for fluent configuration:

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(256)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)
    .add_file_printer("app.log", LogLevel::Info)
    .build();
```

### Configuration Options

| Configuration Item              | Type                 | Default   | Description                                      |
|---------------------------------|----------------------|-----------|--------------------------------------------------|
| `set_buffer_size(size)`         | `size_t`             | `256`     | Ring buffer size (must be power of two)          |
| `set_overflow_policy(policy)`   | `OverflowPolicy`     | `Discard` | Overflow policy: `Discard` or `Block`            |
| `add_console_printer(level)`    | `LogLevel`           | `Info`    | Add console printer, set minimum log level       |
| `add_file_printer(path, level)` | `string`, `LogLevel` | `Debug`   | Add file printer, set path and minimum level     |
| `add_null_printer()`            | -                    | -         | Add Null printer (discard all logs, for testing) |

### Console Printer Configuration

| Configuration | Type | Default          | Description                        |
|---------------|------|------------------|------------------------------------|
| `type`        | enum | -                | Fixed value `PrinterType::Console` |
| `min_level`   | enum | `LogLevel::Info` | Minimum log level                  |

### File Printer Configuration

| Configuration | Type    | Default           | Description                                       |
|---------------|---------|-------------------|---------------------------------------------------|
| `type`        | enum    | -                 | Fixed value `PrinterType::File`                   |
| `min_level`   | enum    | `LogLevel::Info`  | Minimum log level                                 |
| `file_path`   | string  | -                 | Log file path                                     |
| `max_size`    | integer | `0` (no rotation) | Maximum file size in bytes, rotates when exceeded |
| `flush_every` | integer | `64`              | Flush after every N writes                        |

---

## API Reference

### Builder API

```cpp
auto logger = LoggerBuilder()
    .set_buffer_size(256)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)
    .build();
```

#### Method Descriptions

| Method                          | Parameters       | Default | Description                            |
|---------------------------------|------------------|---------|----------------------------------------|
| `set_buffer_size(size)`         | size_t           | 256     | Set ring buffer size                   |
| `set_overflow_policy(policy)`   | OverflowPolicy   | Discard | Set overflow policy                    |
| `add_console_printer(level)`    | LogLevel         | Info    | Add console printer with minimum level |
| `add_file_printer(path, level)` | string, LogLevel | Debug   | Add file printer                       |
| `add_null_printer()`            | -                | -       | Add Null printer                       |

#### Default Configuration

```cpp
LoggerRef create_default_logger();
```

Creates a LoggerRef with default configuration: Console Printer + Info level + Discard overflow policy.

**Example:**
```cpp
auto logger = create_default_logger();
logger.info("Hello");
```

#### LoggerRef

`LoggerRef` is a wrapper class for Logger that supports **copy semantics**.

**Example:**
```cpp
auto logger1 = LoggerBuilder().add_console_printer().build();
auto logger2 = logger1;  // Copy, shares the same Logger

logger1.info("Message from logger1");
logger2.info("Message from logger2");  // Same Logger
```

#### Shutting Down Logger

The Logger automatically calls `shutdown()` upon destruction.

### Log Levels

Ordered from lowest to highest:

| Level   | Description  | Suggested Use Case                       |
|---------|--------------|------------------------------------------|
| `Debug` | Debug info   | Detailed tracing during development      |
| `Info`  | General info | Normal runtime status recording          |
| `Warn`  | Warning      | Potential issues but不影响正常运行              |
| `Error` | Error        | Exceptional situations but不影响正常运行        |
| `Fatal` | Fatal error  | Errors that prevent program continuation |

**Level filtering rule:** A log is recorded only if its level ≥ the printer's configured `min_level`.

### Formatted Output

TinyLogger uses `fmt` library syntax:

```cpp
logger.info("User {} logged in", username);
logger.warn("Warning: memory usage {}%", 85);
logger.debug("Value: {:.2f}", 3.14159);
logger.error("Error code: {:#x}", 0xDEAD);
logger.info("Multiple values: {}, {}, {}", a, b, c);
```

For detailed syntax, refer to the [fmt documentation](https://fmt.dev/latest/syntax.html).

Note: Log parameters must be copyable small POD types (e.g., integers, floats, C-strings `const char*`, etc.) with total
size不超过fixed storage (default 128B). Avoid passing temporary objects or large objects like `std::string`.

---

## Advanced Usage

### Multiple Printer Configuration

You can configure multiple output targets using the Builder:

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(512)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)    // Console: Debug+
    .add_file_printer("app.log", LogLevel::Info)   // File: Info+
    .build();
```

This configuration will:

- Output all Debug+ logs to the console
- Record Info+ logs in `app.log`

### Log File Rotation

The Builder supports file log configuration (detailed parameters not yet exposed in Builder; use LoggerConfig directly):

```cpp
PrinterConfig file_cfg;
file_cfg.type = PrinterType::File;
file_cfg.file_path = "app.log";
file_cfg.max_size = 10485760;  // 10 MB
file_cfg.flush_every = 64;

LoggerConfig config;
config.buffer_size = 256;
config.printers.push_back(file_cfg);

auto logger = LoggerBuilder().set_config(config).build();
```

---

## Using in Other Projects

TinyLogger supports three integration methods.

### Method 1: System Installation (find_package)

Build and install TinyLogger, then use `find_package`:

```bash
git clone https://github.com/2059353675/TinyLogger.git
cd TinyLogger
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
sudo cmake --install .
```

```cmake
find_package(TinyLogger REQUIRED)
target_link_libraries(your_target TinyLogger::tinylogger)
```

### Method 2: Source Integration (add_subdirectory)

Place TinyLogger in your project directory and add it as a subdirectory. TinyLogger will automatically download fmt via `FetchContent`:

```cmake
add_subdirectory(path/to/TinyLogger)
target_link_libraries(your_target TinyLogger::tinylogger)
```

To use the system-installed fmt instead, set `USE_SYSTEM_FMT` before adding the subdirectory:

```cmake
set(USE_SYSTEM_FMT ON)
add_subdirectory(path/to/TinyLogger)
target_link_libraries(your_target TinyLogger::tinylogger)
```

### Method 3: FetchContent

Download TinyLogger automatically during CMake configuration:

```cmake
include(FetchContent)
FetchContent_Declare(
    TinyLogger
    GIT_REPOSITORY https://github.com/2059353675/TinyLogger.git
    GIT_TAG main
)
FetchContent_MakeAvailable(TinyLogger)
target_link_libraries(your_target TinyLogger::tinylogger)
```

To use the system-installed fmt:

```cmake
set(USE_SYSTEM_FMT ON)
FetchContent_MakeAvailable(TinyLogger)
target_link_libraries(your_target TinyLogger::tinylogger)
```

---

## Frequently Asked Questions

### Q: Logs are not being written to the file?

Check:

1. Does the File Printer's `file_path` have write permissions?
2. Is the `min_level` configured correctly (may be filtering)?
3. Have you called `shutdown()` or waited for automatic flush?

### Q: How to change the overflow policy?

Use the Builder to set it (cannot be changed after initialization):

```cpp
auto logger = LoggerBuilder()
    .set_overflow_policy(OverflowPolicy::Discard)  // Discard new logs (default, better performance)
    // or
    .set_overflow_policy(OverflowPolicy::Block)   // Block until space available (guarantees no log loss)
    .add_console_printer()
    .build();
```

### Q: Compiler cannot find the fmt library?

Ensure the fmt library is installed and specify the path during compilation:

```bash
g++ ... -L/usr/lib -lfmt
```

Or use CMake to find it automatically.

---

## Example Program

After building the project, the example program is located at `outputs/example`.

---

**For more information, refer to:**

- [Developer Documentation](DEVELOPER.md) - Build system, testing, contribution guide