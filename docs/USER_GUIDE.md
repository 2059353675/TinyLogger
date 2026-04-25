# TinyLogger 用户指南

> 面向使用者：快速了解如何使用 TinyLogger 库

## 目录

- [快速开始](#快速开始)
  - [安装依赖](#安装依赖)
  - [构建与安装](#构建与安装)
  - [基本使用](#基本使用)
- [配置方式](#配置方式)
  - [程序化配置](#程序化配置推荐)
  - [配置项说明](#配置项说明)
- [API 参考](#api-参考)
  - [Logger 类](#logger-类)
  - [日志级别](#日志级别)
  - [格式化输出](#格式化输出)
- [高级用法](#高级用法)
  - [多 Printer 配置](#多-printer-配置)
  - [文件日志滚动](#文件日志滚动)
- [在其他项目中使用](#在其他项目中使用)
- [常见问题](#常见问题)

---

## 快速开始

### 安装依赖

TinyLogger 需要以下依赖：

- **C++17 兼容编译器**（GCC 9+, Clang 10+, MSVC 2019+）
- **fmt 库**：高性能格式化输出
- **nlohmann/json 库**：JSON 解析

#### Ubuntu/Debian

```bash
sudo apt-get install libfmt-dev nlohmann-json3-dev
```

#### Arch Linux

```bash
sudo pacman -S fmt nlohmann-json
```

#### Fedora

```bash
sudo dnf install fmt-devel nlohmann-json-devel
```

### 构建与安装

```bash
# 克隆项目
git clone <repository-url>
cd TinyLogger

# 构建
mkdir build && cd build
cmake ..
make

# 安装（可选）
sudo make install
```

构建产物：
- `build/libTinyLogger.a` - 静态库
- `build/example` - 示例程序
- `build/test/*` - 测试可执行文件

### 基本使用

推荐使用 `LoggerBuilder`（链式配置，类型安全）：

```cpp
#include <tiny_logger/logger_builder.h>

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

或使用默认配置（最简方式）：

```cpp
#include <tiny_logger/logger_builder.h>

int main() {
    auto logger = tiny_logger::create_default_logger();
    logger.info("应用程序启动");
    return 0;
}
```

编译你的程序：

```bash
g++ -std=c++17 -I/path/to/TinyLogger/include -o myapp myapp.cpp \
    -L/path/to/TinyLogger/build -lTinyLogger -lfmt
```

---

## 配置方法

使用 `LoggerBuilder` 进行链式配置（推荐）：

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(256)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)
    .add_file_printer("app.log", LogLevel::Info)
    .build();
```

### 配置项说明

| 配置项 | 类型 | 说明 |
|--------|------|------|
| `set_buffer_size(size)` | 函数 | 环形缓冲区大小（槽位数），必须是 2 的幂次（默认 256） |
| `set_overflow_policy(policy)` | 函数 | 溢出策略：`Discard`（丢弃）或 `Block`（阻塞） |
| `add_console_printer(level)` | 函数 | 添加控制台输出，参数为最小日志级别 |
| `add_file_printer(path, level)` | 函数 | 添加文件输出，参数为文件路径和最小日志级别 |
| `add_null_printer()` | 函数 | 添加 Null 输出（丢弃所有日志，用于测试） |

### Console Printer 配置

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `type` | 枚举 | - | 固定值 `PrinterType::Console` |
| `min_level` | 枚举 | `LogLevel::Info` | 最低日志级别 |

### File Printer 配置

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `type` | 枚举 | - | 固定值 `PrinterType::File` |
| `min_level` | 枚举 | `LogLevel::Info` | 最低日志级别 |
| `file_path` | 字符串 | - | 日志文件路径 |
| `max_size` | 整数 | `0`（不滚动） | 文件最大字节数，超出后滚动 |
| `flush_every` | 整数 | `64` | 每 N 次写入后 flush |

---

## API 参考

### Builder API

```cpp
auto logger = LoggerBuilder()
    .set_buffer_size(256)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)
    .build();
```

#### 方法说明

| 方法 | 参数 | 默认值 | 说明 |
|------|------|--------|------|
| `set_buffer_size(size)` | size_t | 256 | 环形缓冲区大小 |
| `set_overflow_policy(policy)` | OverflowPolicy | Discard | 溢出策略 |
| `add_console_printer(level)` | LogLevel | Info | 添加控制台，设定最小级别 |
| `add_file_printer(path, level)` | string, LogLevel | Debug | 添加文件输出 |
| `add_null_printer()` | - | - | 添加 Null Printer |

#### 默认配置

```cpp
LoggerRef create_default_logger();
```

使用默认配置创建 LoggerRef：Console Printer + Info 级别 + Discard 溢出策略。

**示例：**
```cpp
auto logger = create_default_logger();
logger.info("Hello");
```

#### LoggerRef

`LoggerRef` 是 Logger 的包装类，支持**拷贝语义**

**示例：**
```cpp
auto logger1 = LoggerBuilder().add_console_printer().build();
auto logger2 = logger1;  // 拷贝，共享同一 Logger

logger1.info("Message from logger1");
logger2.info("Message from logger2");  // 同一 Logger
```

#### 关闭 Logger

Logger 在析构时会自动调用 `shutdown()`。

### 日志级别

从低到高排列：

| 级别 | 说明 | 适用场景（建议） |
|------|------|----------|
| `Debug` | 调试信息 | 开发阶段详细追踪 |
| `Info` | 一般信息 | 正常运行状态记录 |
| `Warn` | 警告信息 | 潜在问题但不影响运行 |
| `Error` | 错误信息 | 异常情况但不影响运行 |
| `Fatal` | 致命错误 | 导致程序无法继续的错误 |

**级别过滤规则：** 只有当日志级别 ≥ Printer 配置的 `level` 时，才会被记录。

### 格式化输出

TinyLogger 使用 `fmt` 库语法：

```cpp
logger.info("用户 {} 登录", username);
logger.warn("警告：内存使用率 {}%", 85);
logger.debug("数值：{:.2f}", 3.14159);
logger.error("错误码：{:#x}", 0xDEAD);
logger.info("多个值：{}, {}, {}", a, b, c);
```

详细语法参考：[fmt 文档](https://fmt.dev/latest/syntax.html)

注意：日志参数必须是可拷贝的小型 POD 类型（如整数、浮点、C 字符串 `const char*` 等），且总大小不超过固定存储（默认 128B），避免传入临时对象或大对象，如 `std::string`。

---

## 高级用法

### 多 Printer 配置

可以使用 Builder 同时配置多个输出目标：

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(512)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)    // 控制台：Debug+
    .add_file_printer("app.log", LogLevel::Info)   // 文件：Info+
    .build();
```

此配置会：
- 在控制台输出所有 Debug+ 日志
- 在 `app.log` 记录 Info+ 日志

### 文件日志滚动

Builder 支持文件日志配置（详细参数暂未在 Builder 中暴露，可直接使用 LoggerConfig）：

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

## 在其他项目中使用

安装 TinyLogger 后，可以在其他项目的 CMakeLists.txt 中使用：

```cmake
find_package(TinyLogger REQUIRED)
target_link_libraries(your_target TinyLogger::tinylogger)
```

---

## 常见问题

### Q: 日志没有写入文件？

检查：
1. File Printer 的 `file_path` 是否有写权限
2. `min_level` 设置是否正确（可能被过滤）
3. 是否调用了 `shutdown()` 或等待自动 flush

### Q: 如何更改溢出策略？

使用 Builder 设置（初始化后不可更改）：

```cpp
auto logger = LoggerBuilder()
    .set_overflow_policy(OverflowPolicy::Discard)  // 丢弃新日志（默认，性能更好）
    // 或
    .set_overflow_policy(OverflowPolicy::Block)   // 阻塞等待（保证不丢日志）
    .add_console_printer()
    .build();
```

### Q: 编译时找不到 fmt 库？

确保已安装 fmt 库，并在编译时指定路径：

```bash
g++ ... -L/usr/lib -lfmt
```

或使用 CMake 自动查找。

---

## 示例程序

构建项目后，示例程序位于 `build/example`：

```bash
./build/example
```

---

**更多信息请参考：**
- [开发者文档](DEVELOPER.md) - 构建系统、测试、贡献指南
