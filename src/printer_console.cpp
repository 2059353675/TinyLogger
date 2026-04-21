#include "TinyLogger/printer_console.h"
#include <fmt/format.h>

namespace tiny_logger {

ConsolePrinter::ConsolePrinter(const PrinterConfig& config) {
    min_level_ = config.min_level;
    type_ = PrinterType::Console;
}

void ConsolePrinter::write(LogEvent& event) {
    fmt::memory_buffer buf;
    format_log_line(event, buf);

    std::fwrite(buf.data(), 1, buf.size(), stdout);

    if (buf.size() == 0 || buf.data()[buf.size() - 1] != '\n') {
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