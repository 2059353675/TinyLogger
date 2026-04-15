#include "TinyLogger/printer_file.h"

namespace TinyLogger {

FilePrinter::FilePrinter(const PrinterConfig& config) {
    min_level_ = config.min_level;

    const auto& j = config.raw;

    if (!j.contains("path")) {
        throw std::runtime_error("FilePrinter requires 'path'");
    }
    file_path_ = j["path"].get<std::string>();

    if (j.contains("max_size")) {
        max_size_ = j["max_size"].get<size_t>();
    }

    if (j.contains("flush_every")) {
        flush_every_ = j["flush_every"].get<size_t>();
    }

    open_file();
}

FilePrinter::~FilePrinter() {
    if (file_) {
        flush();
        std::fclose(file_);
    }
}

void FilePrinter::write(const LogEvent& event) {
    if (!file_)
        return;

    std::string ts = format_timestamp(event.timestamp);

    std::string line = fmt::format(
        "[{}][{}][{}] {}", ts, event.thread_id, level_to_string(event.level), std::string_view(event.buffer, event.length));

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
    std::fclose(file_);

    std::string backup = file_path_ + ".1";
    if (std::rename(file_path_.c_str(), backup.c_str()) != 0) {
        std::fprintf(stderr, "[TinyLogger] Failed to rotate log file: %s\n", file_path_.c_str());
    }

    open_file();
    current_size_ = 0;
}

void register_file_printer() {
    TinyLogger::PrinterRegistry::instance().register_printer(
        TinyLogger::PrinterType::File,
        [](const TinyLogger::PrinterConfig& cfg) { return std::make_unique<TinyLogger::FilePrinter>(cfg); });
}
} // namespace TinyLogger