# TinyLogger 示例

## 文件说明

- `example.cpp` - 示例程序源码
- `logger_config.json` - 配置文件示例
- `Makefile` - 构建脚本

## 快速开始

### 前提条件

确保已经构建了 TinyLogger 库，`libTinyLogger.a` 位于 `../build/` 目录中。

```bash
# 在项目根目录执行
mkdir -p build && cd build
cmake .. && make
cd ../examples
```

### 编译和运行

```bash
# 编译
make

# 运行
make run
```

或者手动编译：

```bash
g++ -std=c++17 -I../include -o example example.cpp -L../build -lTinyLogger -lfmt
./example
```

## 配置说明

`logger_config.json` 示例配置：

```json
{
    "buffer_size": 512,          // 环形缓冲区大小（槽位数）
    "overflow_policy": "Discard", // 溢出策略：Discard 或 Block
    "printers": [
        {
            "type": "Console",    // 控制台输出
            "min_level": "Debug"  // 最低日志级别
        },
        {
            "type": "File",       // 文件输出
            "min_level": "Info",
            "file_path": "example.log",
            "max_size": 1048576,  // 文件最大字节数（滚动）
            "flush_every": 64     // 每 N 次写入后 flush
        }
    ]
}
```

### 日志级别

从低到高：`Debug` < `Info` < `Error` < `Fatal`

## API 使用示例

```cpp
#include <TinyLogger/logger.h>

TinyLogger::Logger logger;
logger.init("logger_config.json");

// 基本用法
logger.info("这是一条信息");
logger.debug("调试信息：{}", 42);
logger.error("错误：{}", "详细信息");
logger.fatal("致命错误：{}", 错误码);

// 格式化输出（使用 fmt 库语法）
logger.info("用户 {} 登录，IP: {}", username, ip_address);
logger.debug("数值：{:.2f}", 3.14159);
```
