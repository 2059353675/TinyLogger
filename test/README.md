# TinyLogger 测试套件

本目录包含 TinyLogger 项目的完整测试用例。

## 测试结构

```
test/
├── configs/                  # 测试用配置文件
│   ├── test_console.json    # Console printer 配置
│   ├── test_file.json       # File printer 配置
│   ├── test_multi.json      # 多 printer 配置
│   └── test_overflow.json   # 溢出策略配置
├── test_ring_buffer.cpp     # RingBuffer 单元测试
├── test_config.cpp          # Config 配置解析测试
├── test_printer.cpp         # Printer (Console/File) 测试
├── test_distributor.cpp     # Distributor 测试
├── test_logger.cpp          # Logger 集成测试
├── CMakeLists.txt           # CMake 构建配置
├── run_tests.bat            # Windows 测试运行脚本
└── run_tests.sh             # Linux/macOS 测试运行脚本
```

## 测试覆盖范围

### 1. RingBuffer 测试 (test_ring_buffer.cpp)

**基础测试：**
- 缓冲区创建
- 单次入队/出队
- 空缓冲区出队返回 false
- 容量限制 enforcement
- FIFO 顺序保证

**多级测试：**
- 多日志级别入队
- 大消息处理（接近 LOG_MSG_SIZE）

**并发测试：**
- 单生产者-单消费者并发
- 多生产者-单消费者并发

**边界测试：**
- 2 的幂次容量验证
- 缓冲区循环覆盖（wraparound）

### 2. Config 测试 (test_config.cpp)

**基础测试：**
- 最小有效配置
- 完整配置（所有选项）

**错误处理测试：**
- 文件不存在
- 无效 JSON 语法
- 缺少 printers 数组
- 无效 printer type
- 无效 overflow policy
- 无效 log level
- 非 2 的幂 buffer size
- File printer 缺少 path

**边界情况测试：**
- 大小写不敏感解析
- 可选字段默认值
- 空 printers 数组
- 多个相同 type 的 printers

### 3. Printer 测试 (test_printer.cpp)

**ConsolePrinter 测试：**
- 初始化
- 写入消息
- 多行消息

**FilePrinter 测试：**
- 初始化和文件创建
- 单次写入
- 多次写入
- Flush 功能
- 文件滚动（rotation）
- 追加模式

**级别过滤测试：**
- 日志级别过滤
- 日志级别顺序验证

**异常处理测试：**
- 无效路径
- 空消息

### 4. Distributor 测试 (test_distributor.cpp)

**基础测试：**
- 创建 Distributor
- 启动/停止
- 添加 Printer

**分发测试：**
- 单个事件分发
- 多个事件分发
- 多 Printer 分发

**级别过滤测试：**
- 基于级别的过滤

**并发测试：**
- 并发入队

**生命周期测试：**
- 停止时清空缓冲区
- 停止时 flush
- 双重启动/停止

**批量处理测试：**
- 批量事件处理

### 5. Logger 集成测试 (test_logger.cpp)

**初始化测试：**
- 有效配置初始化
- 无效配置处理
- File printer 初始化

**日志记录测试：**
- info/debug/error/fatal 日志
- 格式化输出

**级别过滤测试：**
- 日志级别过滤

**并发测试：**
- 多线程并发日志

**溢出策略测试：**
- Discard 策略

**多 Printer 测试：**
- 同时输出到多个目标

**生命周期测试：**
- 启动/停止周期

## 运行测试

### Windows

```bash
cd test
run_tests.bat
```

### Linux/macOS

```bash
cd test
chmod +x run_tests.sh
./run_tests.sh
```

### 手动构建和运行

```bash
# 创建构建目录
cd test
mkdir build && cd build

# 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build . -j$(nproc)

# 运行单个测试
./test_ring_buffer
./test_config
./test_printer
./test_distributor
./test_logger

# 运行所有测试（使用 CTest）
ctest --output-on-failure
```

## 依赖项

测试需要以下依赖：
- CMake 3.14+
- C++17 编译器
- fmt 库
- nlohmann/json 库

## 添加新测试

1. 在相应的测试文件中添加测试函数
2. 测试函数应返回 `bool`（true=通过，false=失败）
3. 在 `main()` 函数中注册测试
4. 遵循现有的命名和格式约定

## 测试约定

- 每个测试函数以 `test_` 开头
- 测试输出格式：`[TEST] 测试名称... PASSED/FAILED`
- 使用工具函数创建临时配置文件
- 测试结束后清理临时文件
- 并发测试使用适当的睡眠和等待机制
