# TinyLogger 使用者指南

**[English](./USER_GUIDE.md)** | **[简体中文](./USER_GUIDE.zh-Hans.md)** | **[繁體中文](./USER_GUIDE.zh-Hant.md)**

> 面向使用者：快速了解如何使用 TinyLogger 函式庫

## 目錄

- [快速開始](#快速開始)
    - [安裝依賴](#安裝依賴)
    - [建置與安裝](#建置與安裝)
    - [最小範例](#最小範例)
- [設定方法](#設定方法)
    - [設定項目說明](#設定項目說明)
    - [Console Printer 設定](#console-printer-設定)
    - [File Printer 設定](#file-printer-設定)
- [API 參考](#api-參考)
    - [Builder API](#builder-api)
    - [日誌等級](#日誌等級)
    - [格式化輸出](#格式化輸出)
- [進階用法](#進階用法)
    - [執行緒命名](#執行緒命名)
    - [多 Printer 設定](#多-printer-設定)
    - [檔案日誌滾動](#檔案日誌滾動)
- [在其他專案中使用](#在其他專案中使用)
- [常見問題](#常見問題)

---

## 快速開始

### 安裝依賴

TinyLogger 需要以下依賴：

- **C++17 相容編譯器**（GCC 9+, Clang 10+, MSVC 2019+）
- **CMake 建置工具**：至少 3.14
- **fmt 函式庫**：高效能格式化輸出

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

### 建置與安裝

```bash
# 克隆專案
git clone https://github.com/2059353675/TinyLogger.git
cd TinyLogger

# 建置
mkdir build && cd build
cmake ..
cmake --build .

# 安裝（可選）
sudo cmake --install .
```

建置產物：

- `outputs/libTinyLogger.a` - 靜態函式庫
- `outputs/example` - 範例程式
- `outputs/test_*` - 測試可執行檔

### 最小範例

推薦使用 `LoggerBuilder`（鏈式設定，型別安全）：

```cpp
#include <tiny_logger/logger_builder.hpp>

int main() {
    using namespace tiny_logger;

    auto logger = LoggerBuilder()
        .set_buffer_size(256)
        .set_overflow_policy(OverflowPolicy::Discard)
        .add_console_printer(LogLevel::Debug)
        .build();

    logger.info("應用程式啟動");
    logger.debug("除錯資訊：{}", 42);
    logger.error("錯誤：{}", "詳細資訊");

    return 0;
}
```

或使用預設設定（僅中斷，輸出 Info 以上等級日誌）：

```cpp
#include <tiny_logger/logger_builder.hpp>

int main() {
    auto logger = tiny_logger::create_default_logger();
    logger.info("應用程式啟動");
    return 0;
}
```

然後編譯你的程式：

```bash
g++ -std=c++17 -I /path/to/TinyLogger/include -o myapp myapp.cpp \
    -L /path/to/TinyLogger/build -lTinyLogger -lfmt
```

---

## 設定方法

使用 `LoggerBuilder` 進行鏈式設定：

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(256)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)
    .add_file_printer("app.log", LogLevel::Info)
    .build();
```

### 設定項目說明

| 設定項目                            | 型別                   | 預設值       | 說明                              |
|---------------------------------|----------------------|-----------|---------------------------------|
| `set_buffer_size(size)`         | `size_t`             | `256`     | 環形緩衝區大小（必須為 2 的冪次）              |
| `set_overflow_policy(policy)`   | `OverflowPolicy`     | `Discard` | 溢位策略：`Discard`（丟棄）或 `Block`（阻塞） |
| `add_console_printer(level)`    | `LogLevel`           | `Info`    | 新增主控台輸出，設定最低日誌等級                |
| `add_file_printer(path, level)` | `string`, `LogLevel` | `Debug`   | 新增檔案輸出，設定路徑與最低等級                |
| `add_null_printer()`            | -                    | -         | 新增 Null Printer（丟棄所有日誌，常用於測試）   |

### Console Printer 設定

| 設定項目        | 型別 | 預設值              | 說明                         |
|-------------|----|------------------|----------------------------|
| `type`      | 列舉 | -                | 固定值 `PrinterType::Console` |
| `min_level` | 列舉 | `LogLevel::Info` | 最低日誌等級                     |

### File Printer 設定

| 設定項目          | 型別 | 預設值              | 說明                      |
|---------------|----|------------------|-------------------------|
| `type`        | 列舉 | -                | 固定值 `PrinterType::File` |
| `min_level`   | 列舉 | `LogLevel::Info` | 最低日誌等級                  |
| `file_path`   | 字串 | -                | 日誌檔案路徑                  |
| `max_size`    | 整數 | `0`（不滾動）         | 檔案最大位元組數，超出後滾動          |
| `flush_every` | 整數 | `64`             | 每 N 次寫入後 flush          |

---

## API 參考

### Builder API

```cpp
auto logger = LoggerBuilder()
    .set_buffer_size(256)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)
    .build();
```

#### 方法說明

| 方法                              | 參數               | 預設值     | 說明              |
|---------------------------------|------------------|---------|-----------------|
| `set_buffer_size(size)`         | size_t           | 256     | 環形緩衝區大小         |
| `set_overflow_policy(policy)`   | OverflowPolicy   | Discard | 溢位策略            |
| `add_console_printer(level)`    | LogLevel         | Info    | 新增主控台，設定最小等級    |
| `add_file_printer(path, level)` | string, LogLevel | Debug   | 新增檔案輸出          |
| `add_null_printer()`            | -                | -       | 新增 Null Printer |

#### 預設設定

```cpp
LoggerRef create_default_logger();
```

使用預設設定建立 LoggerRef：Console Printer + Info 等級 + Discard 溢位策略。

**範例：**

```cpp
auto logger = create_default_logger();
logger.info("Hello");
```

#### LoggerRef

`LoggerRef` 是 Logger 的包裝類別，支援**拷貝語意**

**範例：**

```cpp
auto logger1 = LoggerBuilder().add_console_printer().build();
auto logger2 = logger1;  // 拷貝，共用同一個 Logger

logger1.info("來自 logger1 的訊息");
logger2.info("來自 logger2 的訊息");  // 同一個 Logger
```

#### 關閉 Logger

Logger 在解構時會自動呼叫 `shutdown()`。

### 日誌等級

從低到高排列：

| 等級      | 說明   | 適用場景（建議）    |
|---------|------|-------------|
| `Debug` | 除錯資訊 | 開發階段詳細追蹤    |
| `Info`  | 一般資訊 | 正常運行狀態記錄    |
| `Warn`  | 警告資訊 | 潛在問題但不影響運行  |
| `Error` | 錯誤資訊 | 異常情況但不影響運行  |
| `Fatal` | 致命錯誤 | 導致程式無法繼續的錯誤 |

**等級過濾規則：** 只有當日誌等級 ≥ Printer 設定的 `level` 時，才會被記錄。

### 格式化輸出

TinyLogger 使用 `fmt` 函式庫語法：

```cpp
logger.info("使用者 {} 登入", username);
logger.warn("警告：記憶體使用率 {}%", 85);
logger.debug("數值：{:.2f}", 3.14159);
logger.error("錯誤碼：{:#x}", 0xDEAD);
logger.info("多個值：{}, {}, {}", a, b, c);
```

詳細語法參考：[fmt 文件](https://fmt.dev/latest/syntax.html)

注意：日誌參數必須是可拷貝的小型 POD 型別（如整數、浮點、C 字串 `const char*` 等），且總大小不超過固定儲存空間（預設
128B），避免傳入暫時物件或大型物件，如 `std::string`。

---

## 進階用法

### 執行緒命名

預設情況下，每條日誌包含一個雜湊後的執行緒 ID。你可以使用 `set_thread_name()` 為執行緒設定一個可讀的名稱：

```cpp
#include <tiny_logger/logger_builder.hpp>

void worker() {
    tiny_logger::set_thread_name("Worker-1");

    auto logger = tiny_logger::create_default_logger();
    logger.info("任務開始");
    // 輸出：[2025-01-15 10:30:45.123456][Worker-1][Info] 任務開始
}

int main() {
    tiny_logger::set_thread_name("MainThread");
    // ...
}
```

| 函式                                       | 說明                              |
|--------------------------------------------|-----------------------------------|
| `set_thread_name(const char* name)`        | 設定當前執行緒的顯示名稱             |
| `get_thread_name()`                        | 取得當前執行緒名稱（未設定時為 nullptr） |
| `resolve_thread_name(uint64_t thread_id)`  | 根據執行緒 ID 查詢對應名稱           |

- 呼叫 `set_thread_name(nullptr)` 或 `set_thread_name("")` 會清除名稱。
- 執行緒名稱在日誌記錄時擷取，輸出時顯示為 `[Name]` 而非 `[thread_id]`。
- 未設定時，回退顯示雜湊後的數值執行緒 ID。

### 多 Printer 設定

可以使用 Builder 同時設定多個輸出目標：

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(512)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)    // 主控台：Debug+
    .add_file_printer("app.log", LogLevel::Info)   // 檔案：Info+
    .build();
```

此設定會：

- 在主控台輸出所有 Debug+ 日誌
- 在 `app.log` 記錄 Info+ 日誌

### 檔案日誌滾動

Builder 支援檔案日誌設定（詳細參數暫未在 Builder 中暴露，可直接使用 LoggerConfig）：

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

## 在其他專案中使用

TinyLogger 支援三種整合方式。

### 方法一：系統安裝（find_package）

編譯並安裝 TinyLogger，然後使用 `find_package`：

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

### 方法二：原始碼整合（add_subdirectory）

將 TinyLogger 放入你的專案中，透過 `add_subdirectory` 引入。TinyLogger 會自動透過 `FetchContent` 下載 fmt：

```cmake
add_subdirectory(path/to/TinyLogger)
target_link_libraries(your_target TinyLogger::tinylogger)
```

如需使用系統已安裝的 fmt，請在新增子目錄前設定 `USE_SYSTEM_FMT`：

```cmake
set(USE_SYSTEM_FMT ON)
add_subdirectory(path/to/TinyLogger)
target_link_libraries(your_target TinyLogger::tinylogger)
```

### 方法三：FetchContent 自動下載

在 CMake 配置階段自動下載 TinyLogger：

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

如需使用系統已安裝的 fmt：

```cmake
set(USE_SYSTEM_FMT ON)
FetchContent_MakeAvailable(TinyLogger)
target_link_libraries(your_target TinyLogger::tinylogger)
```

---

## 常見問題

### Q: 日誌沒有寫入檔案？

檢查：

1. File Printer 的 `file_path` 是否有寫入權限
2. `min_level` 設定是否正確（可能被過濾）
3. 是否呼叫了 `shutdown()` 或等待自動 flush

### Q: 如何更改溢位策略？

使用 Builder 設定（初始化後不可更改）：

```cpp
auto logger = LoggerBuilder()
    .set_overflow_policy(OverflowPolicy::Discard)  // 丟棄新日誌（預設，效能較佳）
    // 或
    .set_overflow_policy(OverflowPolicy::Block)   // 阻塞等待（保證不遺失日誌）
    .add_console_printer()
    .build();
```

### Q: 編譯時找不到 fmt 函式庫？

確保已安裝 fmt 函式庫，並在編譯時指定路徑：

```bash
g++ ... -L/usr/lib -lfmt
```

或使用 CMake 自動尋找。

---

## 範例程式

建置專案後，範例程式位於 `outputs/example`。

---

**更多資訊請參考：**

- [開發者文件](DEVELOPER.md) - 建置系統、測試、貢獻指南