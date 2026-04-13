#include "TinyLogger/printer_console.h"

namespace TinyLogger {

ConsolePrinter::ConsolePrinter(const PrinterConfig& config) {
    min_level_ = config.min_level;
}

void ConsolePrinter::write(const LogEvent& event) {
    if (!should_log(event.level))
        return;

    std::string ts = format_timestamp(event.timestamp);

    std::string line = fmt::format(
        "[{}][{}][{}] {}", ts, event.thread_id, level_to_string(event.level), std::string_view(event.buffer, event.length));

    std::fwrite(line.data(), 1, line.size(), stdout);

    if (line.empty() || line.back() != '\n') {
        std::fwrite("\n", 1, 1, stdout);
    }
}

void ConsolePrinter::flush() {
    std::fflush(stdout);
}

void register_console_printer() {
    TinyLogger::PrinterRegistry::instance().register_printer(
        TinyLogger::PrinterType::Console,
        [](const TinyLogger::PrinterConfig& cfg) { return std::make_unique<TinyLogger::ConsolePrinter>(cfg); });
}
} // namespace TinyLogger