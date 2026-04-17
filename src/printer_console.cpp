#include "TinyLogger/printer_console.h"

namespace tiny_logger {

ConsolePrinter::ConsolePrinter(const PrinterConfig& config) {
    min_level_ = config.min_level;
}

void ConsolePrinter::write(const std::string& formatted, const LogEvent& event) {
    std::string ts = format_timestamp(event.timestamp);

    std::string line = fmt::format("[{}][{}][{}] {}", ts, event.thread_id, level_to_string(event.level), formatted);

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