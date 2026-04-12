#pragma once

#include "TinyLogger/printer.h"
#include <cstdio>
#include <string>

namespace TinyLogger {

class FilePrinter : public Printer
{
public:
    explicit FilePrinter(std::string path) : file_path_(std::move(path)) {
    }

    ~FilePrinter() {
        if (file_) {
            flush();
            std::fclose(file_);
        }
    }

    void init(const json& cfg) override {
        open_file();
    }

    void write(const LogEvent& event) override {
        if (!file_)
            return;

        std::string ts = format_timestamp(event.timestamp);

        std::string line = fmt::format("[{}][{}][{}] {}", ts, event.thread_id, level_to_string(event.level),
                                       std::string_view(event.buffer, event.length));

        size_t written = std::fwrite(line.data(), 1, line.size(), file_);
        current_size_ += written;

        if (line.empty() || line.back() != '\n') {
            std::fwrite("\n", 1, 1, file_);
            current_size_ += 1;
        }

        if (++write_count_ >= flush_every_) {
            flush();
            write_count_ = 0;
        }

        if (max_size_ > 0 && current_size_ >= max_size_) {
            rotate();
        }
    }

    void flush() override {
        if (file_) {
            std::fflush(file_);
        }
    }

    void set_flush_every(size_t n) {
        flush_every_ = n;
    }

    void set_max_size(size_t bytes) {
        max_size_ = bytes;
    }

private:
    void open_file() {
        file_ = std::fopen(file_path_.c_str(), "a");
        if (!file_) {
            // fallback：stderr
            file_ = stderr;
        }

        // 设置用户态缓冲
        std::setvbuf(file_, buffer_, _IOFBF, sizeof(buffer_));
    }

    void rotate() {
        std::fclose(file_);

        std::string backup = file_path_ + ".1";
        std::rename(file_path_.c_str(), backup.c_str());

        open_file();
        current_size_ = 0;
    }

private:
    std::string file_path_;
    FILE* file_{nullptr};

    size_t current_size_{0};
    size_t max_size_{0}; // 0 表示不滚动

    size_t write_count_{0};
    size_t flush_every_{64}; // 每 N 条 flush

    char buffer_[64 * 1024]; // 文件缓冲（64KB）
};

} // namespace TinyLogger