#include "tiny_logger/printer/file.h"

namespace tiny_logger {

FilePrinter::FilePrinter(const PrinterConfig& config) {
    min_level_ = config.min_level;
    type_ = PrinterType::File;

    if (config.file_path.empty()) {
        throw std::runtime_error("FilePrinter requires 'path'");
    }
    file_path_ = config.file_path;
    max_size_ = config.max_size;
    flush_every_ = config.flush_every;

    open_file();
}

FilePrinter::~FilePrinter() {
    if (file_) {
        flush();
        std::fclose(file_);
    }
}

void FilePrinter::write(LogEvent& event) {
    if (!file_)
        return;

    fmt::memory_buffer buf;
    format_log_line(event, buf);

    size_t written = std::fwrite(buf.data(), 1, buf.size(), file_);
    current_size_ += written;

    if (buf.size() == 0 || buf.data()[buf.size() - 1] != '\n') {
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

void FilePrinter::flush() {
    if (file_) {
        std::fflush(file_);
    }
}

void FilePrinter::set_flush_every(size_t n) {
    flush_every_ = n;
}

void FilePrinter::set_max_size(size_t bytes) {
    max_size_ = bytes;
}

void FilePrinter::open_file() {
    file_ = std::fopen(file_path_.c_str(), "a");
    if (!file_) {
        // fallback：stderr
        file_ = stderr;
    }

    // 设置用户态缓冲
    std::setvbuf(file_, buffer_, _IOFBF, sizeof(buffer_));
}

void FilePrinter::rotate() {
    std::string backup = file_path_ + ".1";
    if (std::rename(file_path_.c_str(), backup.c_str()) != 0 && errno != ENOENT) {
        std::fprintf(stderr, "[TinyLogger] Failed to rotate log file: %s\n", file_path_.c_str());
    }

    file_ = std::freopen(file_path_.c_str(), "a", file_);
    if (!file_) {
        file_ = stderr;
    }
    current_size_ = 0;
}

void register_file_printer() {
    tiny_logger::PrinterRegistry::instance().register_printer(
        tiny_logger::PrinterType::File,
        [](const tiny_logger::PrinterConfig& cfg) { return std::make_unique<tiny_logger::FilePrinter>(cfg); });
}
} // namespace tiny_logger