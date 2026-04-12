# TinyLogger 用户指南

> 面向使用者：快速了解如何使用 TinyLogger 库

## 目录

- [快速开始](#快速开始)
  - [安装依赖](#安装依赖)
  - [构建与安装](#构建与安装)
  - [基本使用](#基本使用)
- [配置方式](#配置方式)
  - [JSON 配置文件](#json-配置文件)
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

最简单的使用方式：

```cpp
#include <TinyLogger/logger.h>

int main() {
    TinyLogger::Logger logger;
    
    // 通过配置文件初始化
    logger.init("logger_config.json");
    
    // 记录日志
    logger.info("应用程序启动");
    logger.debug("调试信息：{}", 42); // 支持 fmt 风格格式化
    logger.error("错误：{}", "详细信息");
    
    // 程序结束时自动清理
    return 0;
}
```

编译你的程序：

```bash
g++ -std=c++17 -I/path/to/TinyLogger/include -o myapp myapp.cpp \
    -L/path/to/TinyLogger/build -lTinyLogger -lfmt
```

---

## 配置方式

### JSON 配置文件

TinyLogger 使用 JSON 文件配置日志行为，注意大小写敏感：

```json
{
    "buffer_size": 512,
    "overflow_policy": "Discard",
    "printers": [
        {
            "type": "Console",
            "level": "Debug"
        },
        {
            "type": "File",
            "level": "Info",
            "path": "app.log",
            "max_size": 1048576,
            "flush_every": 64
        }
    ]
}
```

### 配置项说明

#### 全局配置

| 配置项 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| `buffer_size` | 整数 | 是 | 环形缓冲区大小（槽位数），必须是 2 的幂次 |
| `overflow_policy` | 字符串 | 是 | 溢出策略：`Discard`（丢弃）或 `Block`（阻塞） |
| `printers` | 数组 | 是 | Printer 配置列表 |

#### Console Printer

| 配置项 | 类型 | 必需 | 默认值 | 说明 |
|--------|------|------|--------|------|
| `type` | 字符串 | 是 | - | 固定值 `"Console"` |
| `level` | 字符串 | 否 | `"Info"` | 最低日志级别 |

#### File Printer

| 配置项 | 类型 | 必需 | 默认值 | 说明 |
|--------|------|------|--------|------|
| `type` | 字符串 | 是 | - | 固定值 `"File"` |
| `level` | 字符串 | 否 | `"Info"` | 最低日志级别 |
| `path` | 字符串 | 是 | - | 日志文件路径 |
| `max_size` | 整数 | 否 | `0`（不滚动） | 文件最大字节数，超出后滚动 |
| `flush_every` | 整数 | 否 | `64` | 每 N 次写入后 flush |

---

## API 参考

### Logger 类

#### `bool init(const std::string& config_path)`

从 JSON 配置文件初始化 Logger。

**参数：**
- `config_path`：配置文件路径

**返回：**
- `true`：初始化成功
- `false`：初始化失败（文件不存在或格式错误）

**示例：**
```cpp
TinyLogger::Logger logger;
if (!logger.init("config.json")) {
    std::cerr << "无法加载配置文件" << std::endl;
    return 1;
}
```

#### `void shutdown()`

关闭 Logger，停止分发器并刷新所有 Printer。

**示例：**
```cpp
logger.shutdown(); // 程序结束前调用
```

#### 日志记录方法

```cpp
void debug(fmt::format_string<T...> fmt, T&&... args);
void info(fmt::format_string<T...> fmt, T&&... args);
void error(fmt::format_string<T...> fmt, T&&... args);
void fatal(fmt::format_string<T...> fmt, T&&... args);
```

### 日志级别

从低到高排列：

| 级别 | 说明 | 适用场景 |
|------|------|----------|
| `Debug` | 调试信息 | 开发阶段详细追踪 |
| `Info` | 一般信息 | 正常运行状态记录 |
| `Error` | 错误信息 | 异常情况但不影响运行 |
| `Fatal` | 致命错误 | 导致程序无法继续的错误 |

**级别过滤规则：** 只有当日志级别 ≥ Printer 配置的 `level` 时，才会被记录。

### 格式化输出

TinyLogger 使用 `fmt` 库语法：

```cpp
logger.info("用户 {} 登录", username);
logger.debug("数值：{:.2f}", 3.14159);
logger.error("错误码：{:#x}", 0xDEAD);
logger.info("多个值：{}, {}, {}", a, b, c);
```

详细语法参考：[fmt 文档](https://fmt.dev/latest/syntax.html)

---

## 高级用法

### 多 Printer 配置

可以同时输出到多个目标：

```json
{
    "buffer_size": 512,
    "overflow_policy": "Discard",
    "printers": [
        {
            "type": "Console",
            "level": "Debug"
        },
        {
            "type": "File",
            "level": "Info",
            "path": "app.log"
        },
        {
            "type": "File",
            "level": "Error",
            "path": "errors.log"
        }
    ]
}
```

此配置会：
- 在控制台输出所有 Debug+ 日志
- 在 `app.log` 记录 Info+ 日志
- 在 `errors.log` 仅记录 Error+ 日志

### 文件日志滚动

当日志文件达到 `max_size` 时，会自动滚动为 `.1`、`.2` 等备份：

```json
{
    "printers": [
        {
            "type": "File",
            "path": "app.log",
            "max_size": 10485760,  // 10 MB
            "flush_every": 64
        }
    ]
}
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

### Q: 配置文件找不到怎么办？

确保配置文件路径正确，或使用绝对路径：

```cpp
logger.init("/absolute/path/to/config.json");
```

### Q: 日志没有写入文件？

检查：
1. File Printer 的 `path` 是否有写权限
2. `level` 设置是否正确（可能被过滤）
3. 是否调用了 `shutdown()` 或等待自动 flush

### Q: 如何更改溢出策略？

在配置文件中修改 `overflow_policy`：
- `"Discard"`：缓冲区满时丢弃新日志（默认，性能更好）
- `"Block"`：缓冲区满时阻塞等待（保证不丢日志）

### Q: 编译时找不到 fmt 库？

确保已安装 fmt 库，并在编译时指定路径：

```bash
g++ ... -L/usr/lib -lfmt
```

或使用 CMake 自动查找。

---

## 示例程序

查看 `examples/` 目录获取完整示例：

```bash
cd examples
make
./example
```

---

**更多信息请参考：**
- [开发者文档](DEVELOPER.md) - 构建系统、测试、贡献指南
- [GitHub 仓库](<repository-url>) - 源码、问题反馈
