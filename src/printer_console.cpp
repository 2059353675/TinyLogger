#include "TinyLogger/printer_console.h"
#include <fmt/format.h>

namespace tiny_logger {

std::string format_log_line(LogEvent& event) {
    fmt::memory_buffer buf;

    if (event.vtable && event.vtable->format_fn) {
        event.vtable->format_fn(event, buf);
    } else if (event.fmt) {
        fmt::format_to(buf, "{}", event.fmt);
    }

    std::string ts = format_timestamp(event.timestamp);
    return fmt::format(
        "[{}][{}][{}] {}", ts, event.thread_id, level_to_string(event.level), std::string_view(buf.data(), buf.size()));
}

ConsolePrinter::ConsolePrinter(const PrinterConfig& config) {
    min_level_ = config.min_level;
    type_ = PrinterType::Console;
}

void ConsolePrinter::write(LogEvent& event) {
    std::string line = format_log_line(event);

    std::fwrite(line.data(), 1, line.size(), stdout);

    if (line.empty() || line.back() != '\n') {
        std::fwrite("\n", 1, 1, stdout);
    }
}

void ConsolePrinter::flush() {
    std::fflush(stdout);
}

void register_console_printer() {
    tiny_logger::PrinterRegistry::instance().register_printer(
        tiny_logger::PrinterType::Console,
        [](const tiny_logger::PrinterConfig& cfg) { return std::make_unique<tiny_logger::ConsolePrinter>(cfg); });
}
} // namespace tiny_logger