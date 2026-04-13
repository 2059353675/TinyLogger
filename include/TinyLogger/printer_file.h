#pragma once

#include "TinyLogger/printer.h"
#include <cstdio>
#include <string>

namespace TinyLogger {

class FilePrinter : public Printer
{
public:
    explicit FilePrinter(const PrinterConfig& config);

    ~FilePrinter() override;

    void write(const LogEvent& event) override;

    void flush() override;

    void set_flush_every(size_t n);

    void set_max_size(size_t bytes);

private:
    void open_file();

    void rotate();

private:
    std::string file_path_;
    FILE* file_{nullptr};

    size_t current_size_{0};
    size_t max_size_{0}; // 0 表示不滚动

    size_t write_count_{0};
    size_t flush_every_{64}; // 每 N 条 flush

    char buffer_[64 * 1024]; // 文件缓冲（64KB）
};

void register_file_printer();

} // namespace TinyLogger