# TinyLogger 构建说明

## 依赖项

- C++17 兼容编译器 (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.14 或更高版本
- [fmt](https://github.com/fmtlib/fmt) 库
- [nlohmann/json](https://github.com/nlohmann/json) 库

### 安装依赖 (Ubuntu/Debian)

```bash
sudo apt-get install libfmt-dev nlohmann-json3-dev
```

### 安装依赖 (Arch Linux)

```bash
sudo pacman -S fmt nlohmann-json
```

### 安装依赖 (Fedora)

```bash
sudo dnf install fmt-devel nlohmann-json-devel
```

## 构建项目

### 构建库

```bash
mkdir build && cd build
cmake ..
make
```

### 构建并运行示例程序

```bash
mkdir build && cd build
cmake ..
make
./example
```

示例程序会从以下路径加载配置文件：
- 当前目录：`logger_config.json`
- examples 子目录：`examples/logger_config.json`

### 构建并运行测试

```bash
mkdir build && cd build
cmake ..
make run_tests
```

或者使用 CTest：

```bash
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

## CMake 选项

可以通过 `-D` 选项控制构建行为：

- `TINYLOGGER_BUILD_TESTS=ON/OFF` - 是否构建测试（默认：ON）
- `TINYLOGGER_BUILD_EXAMPLES=ON/OFF` - 是否构建示例（默认：ON）

示例：

```bash
cmake -DTINYLOGGER_BUILD_TESTS=OFF ..
```

## 安装

```bash
mkdir build && cd build
cmake ..
make install
```

默认安装到 `/usr/local`，可以通过 `CMAKE_INSTALL_PREFIX` 更改：

```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/tinylogger ..
```

## 在其他项目中使用

安装后，可以在其他项目的 CMakeLists.txt 中使用：

```cmake
find_package(TinyLogger REQUIRED)
target_link_libraries(your_target TinyLogger::tinylogger)
```

## 构建产物

构建完成后，在 `build/` 目录中可以找到以下产物：

- `libTinyLogger.a` - 静态库文件
- `example` - 示例程序（当 `TINYLOGGER_BUILD_EXAMPLES=ON` 时）
- `logger_config.json` - 示例配置文件副本
- `test/test_ring_buffer` - RingBuffer 单元测试
- `test/test_config` - Config 单元测试
- `test/test_printer` - Printer 单元测试
- `test/test_distributor` - Distributor 单元测试
- `test/test_logger` - 集成测试

注意：所有构建产物都位于主构建目录（默认为 `build/`）中，不会在源码目录中生成构建文件。
