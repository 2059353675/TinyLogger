# TinyLogger

**[English](./README.md)** | **[简体中文](./README.zh-Hans.md)** | **[繁體中文](./README.zh-Hant.md)**

一款輕量的 C++17 高效能非同步日誌庫，採用無鎖環形緩衝區與多消費者架構。

## 核心特性

- **異步日誌** - 應用執行緒零阻塞，日誌寫入平均延遲約 130ns（p99 < 400ns）
- **無鎖設計** - SPSC 環形緩衝區（基於 Disruptor 訊息佇列架構），單執行緒吞吐量超 700 萬 logs/s
- **高並行** - 8 執行緒並行寫入可達 1500 萬 logs/s
- **RAII 資源管理** - 執行緒、檔案等資源自動清理
- **多輸出目標** - Console、File、Null Printer，支援同時配置多個
- **類型安全配置** - 鏈式 Builder API，編譯期檢查

## 詳細文件

| 文件 | 說明 |
|------|------|
| [USER_GUIDE.md](docs/USER_GUIDE.md) | 使用者指南，包含完整 API 參考、高階用法、FAQ |
| [DEVELOPER.md](docs/DEVELOPER.md) | 開發者文件，包含建置系統、測試規範、架構設計 |

---

## 快速開始

### 安裝依賴

```bash
# Ubuntu/Debian
sudo apt-get install libfmt-dev nlohmann-json3-dev

# Arch Linux
sudo pacman -S fmt nlohmann-json

# Fedora
sudo dnf install fmt-devel nlohmann-json-devel
```

### 建置與安裝

```bash
git clone <repository-url>
cd TinyLogger
mkdir build && cd build
cmake ..
make
sudo make install      # 可選，安裝到 /usr/local
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

    logger.info("應用程式啟動");
    logger.debug("除錯資訊：{}", 42);
    logger.error("錯誤：{}", "詳細資訊");

    return 0;
}
```

編譯：

```bash
g++ -std=c++17 -I/path/to/TinyLogger/include -o myapp myapp.cpp \
    -L/path/to/TinyLogger/build -lTinyLogger -lfmt
```

---

## 核心概念

### 簡要架構圖

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Logger    │────▶│ RingBuffer  │────▶│ Distributor │────▶│  Printers   │
│  (API)      │     │ (lock-free) │     │ (thread)    │     │ Console/File│
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
```

### 核心元件

| 元件 | 說明 |
|------|------|
| **Logger** | 使用者 API，封裝 LogEvent 並寫入 RingBuffer |
| **RingBuffer** | SPSC 無鎖環形緩衝區，Disruptor 風格 |
| **Distributor** | 專用執行緒，消费日誌事件並分發給 Printers |
| **Printers** | 輸出處理器：Console、File、Null |

### 日誌級別

| 級別 | 說明 | 建議場景 |
|------|------|----------|
| `Debug` | 除錯資訊 | 開發階段詳細追蹤 |
| `Info` | 一般資訊 | 正常運作狀態記錄 |
| `Error` | 錯誤資訊 | 異常但不影響運作 |
| `Fatal` | 致命錯誤 | 程式無法繼續 |

級別過濾規則：只有當日誌級別 ≥ Printer 配置的 `min_level` 時才會被記錄。

---

## 效能基準 (Performance Benchmark)

TinyLogger 採用無鎖環形緩衝區（Lock-free RingBuffer）與異步分發設計，旨在將日誌記錄對業務主執行緒的影響降至最低。以下是一組實測資料，僅供參考：

### 配置

- Buffer Size: 256 slots
- Payload: 128B storage per event
- Iterations: 50,000 measurements (after 10,000 warmup)
- Hardware: 2 核 CPU，Linux 5.15.0-164-generic

### 延遲測試 (Latency)

測量從呼叫 `logger.info()` 到函式回傳（即主執行緒等待部分）的時間開銷。測試使用 `NullPrinter` 以排除磁碟 I/O 的干擾。

| 指標           | 耗時 (ns) | 說明                         |
| ------------------ | ------------- | -------------------------------- |
| 平均延遲 (Avg) | 138.7 ns  | 极速回應，主執行緒開銷微秒級以下   |
| 中位數 (p50)   | 112.0 ns  | 絕大多數呼叫的核心開銷           |
| p99 分位       | 345.0 ns  | 即使在高並行下，長尾延遲依然受控 |
| 最小延遲 (Min) | 56.0 ns       | 理想情況下的最快寫入             |

### 吞吐量測試 (Throughput)

評估系統在持續高負載下的極限處理能力。同樣使用 `NullPrinter` 以排除磁碟 I/O 的干擾。

| 並行執行緒數 | 吞吐量 (ops/s) |
|-----------|----------------|
| 1 執行緒 | 7.08 M |
| 2 執行緒 | 7.31M |
| 4 執行緒 | 14.21 M |
| 8 執行緒 | 15.26 M |

### 關鍵路徑開銷分解

| 環節 | 說明 | 耗時 |
|------|------|------|
| 1. tuple 建構 | 將參數打包到 128B storage | ~71 ns |
| 2. RingBuffer::enqueue | 無鎖入隊 | ~35 ns |
| 3. RingBuffer::dequeue | 無鎖出隊（後台執行緒） | - |
| 4. event.format_fn | 呼叫回呼函式格式化 | - |
| 5. Printer::write | 寫入輸出（後台執行緒） | - |

**關鍵設計：**
- 主執行緒延遲格式化: `logger.log()` 在主執行緒僅執行參數的 tuple 建構（使用 placement new 寫入預分配空間）並入隊。耗時的 `fmt::format()` 字元串接和 I/O 寫入完全由後台執行緒異步執行。
- 無鎖 RingBuffer: `RingBuffer::enqueue()` 每個應用執行緒獨立擁有一個 SPSC 佇列，實現無競爭入隊。
- RAII 資源管理: 確保所有異步任務在系統關閉（shutdown）前被正確刷新和回收。

**效能建議：**
- 生產環境優先使用 `OverflowPolicy::Discard`，避免阻塞延遲尖峰
- 批次日誌寫入時，後台執行緒可充分消化，延遲更穩定

### 復現方式

你可以透過建置系統執行自带的測試套件來驗證這些資料：

```bash
cd build
make run_tests  # 包含單元測試與基準測試
```

---

## 建置與安裝

### CMake 選項

| 選項 | 預設值 | 說明 |
|------|--------|------|
| `TINYLOGGER_BUILD_TESTS` | ON | 建置測試套件 |
| `TINYLOGGER_BUILD_EXAMPLES` | ON | 建置範例程式 |
| `CMAKE_BUILD_TYPE` | - | 建置類型（Release/Debug） |
| `CMAKE_INSTALL_PREFIX` | /usr/local | 安裝路徑 |

### 建置命令

```bash
# 完整建置
mkdir build && cd build && cmake .. && make

# Release 建置（推薦）
mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make

# 僅建置庫
cmake .. -DTINYLOGGER_BUILD_TESTS=OFF -DTINYLOGGER_BUILD_EXAMPLES=OFF

# 僅建置測試
cmake .. -DTINYLOGGER_BUILD_EXAMPLES=OFF

# 僅建置範例
cmake .. -DTINYLOGGER_BUILD_TESTS=OFF

# 自訂安裝路徑
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/tinylogger && make install
```

### 清理

```bash
make clean              # 清理建置產物
make clean-all        # 清理建置 + 測試暫存檔 + 範例產物
```

---

## 配置方式

### Builder 鏈式配置（推薦）

```cpp
using namespace tiny_logger;

auto logger = LoggerBuilder()
    .set_buffer_size(512)
    .set_overflow_policy(OverflowPolicy::Discard)
    .add_console_printer(LogLevel::Debug)        // 控制台輸出
    .add_file_printer("app.log", LogLevel::Info)  // 檔案輸出
    .build();
```

### 設定項說明

| 方法 | 參數 | 預設值 | 說明 |
|------|------|--------|------|
| `set_buffer_size(size)` | size_t | 256 | 環形緩衝區大小（必須是 2 的���次�� |
| `set_overflow_policy(policy)` | OverflowPolicy | Discard | 溢位策略 |
| `add_console_printer(level)` | LogLevel | Info | 新增控制台 Printer |
| `add_file_printer(path, level)` | string, LogLevel | Debug | 新增檔案 Printer |

### 溢位策略

| 策略 | 說明 | 注意事項 |
|------|------|----------|
| `Discard` | 丟棄新日誌（預設） | 效能更好，適合生產環境 |
| `Block` | 阻塞等待空間 | 可能導致 3ms+ 延遲尖峰，慎用 |

### 預設配置

```cpp
auto logger = tiny_logger::create_default_logger();
```

使用預設配置：Console Printer + Info 級別 + Discard 溢位策略。

---

## API 快速參考

### 日誌記錄

```cpp
logger.debug(fmt::format_string<T...>, T&&... args);
logger.info(fmt::format_string<T...>, T&&... args);
logger.error(fmt::format_string<T...>, T&&... args);
logger.fatal(fmt::format_string<T...>, T&&... args);
```

### 範例

```cpp
logger.info("使用者 {} 登入", username);
logger.debug("數值：{:.2f}", 3.14159);
logger.error("錯誤碼：{:#x}", 0xDEAD);
logger.info("多個值：{}, {}, {}", a, b, c);
```

日誌參數應為可拷貝的小型 POD 類型（整數、浮點、C 字元串等），且總大小不超過 128 位元組。

---

## 在其他專案中使用

安裝後，在 CMakeLists.txt 中使用：

```cmake
find_package(TinyLogger REQUIRED)
target_link_libraries(your_target TinyLogger::tinylogger)
```

---

## 專案結構

```
TinyLogger/
├── include/TinyLogger/      # 標頭檔
│   ├── logger.h
│   ├── ring_buffer.h
│   ├── types.h
│   └── ...
├── src/                    # 實作檔
│   ├── logger.cpp
│   ├── ring_buffer.cpp
│   ├── distributor.cpp
│   └── ...
├── test/                   # 測試套件
│   ├── test_ring_buffer.cpp
│   ├── test_printer.cpp
│   ├── test_distributor.cpp
│   └── test_logger.cpp
├── examples/               # 範例程式
│   ├── example.cpp
│   └── speed_test.cpp
├── docs/                   # 詳細文件
│   ├── USER_GUIDE.md
│   └── DEVELOPER.md
└── CMakeLists.txt
```

---

## 常見問題

### Q: 日誌沒有寫入檔案？

檢查：
1. File Printer 的 `file_path` 是否有寫入權限
2. `min_level` 設定是否正確
3. 是否呼叫了 `shutdown()` 或等待自動 flush

### Q: 如何選擇溢位策略？

- **Discard**（預設）：生產環境推薦，效能更好
- **Block**：需要保證不丟日誌時使用，但可能引發延遲尖峰

### Q: 編譯時找不到 fmt 庫？

確保已安裝 fmt 庫，並在編譯時指定路徑：

```bash
g++ ... -L/usr/lib -lfmt
```

或使用 CMake 自動查找。