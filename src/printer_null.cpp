#include "TinyLogger/printer_null.h"

namespace tiny_logger {

NullPrinter::NullPrinter(const PrinterConfig& config) {
    min_level_ = config.min_level;
}

void NullPrinter::write(const std::string& formatted, const LogEvent& event) {
}

void NullPrinter::flush() {
}

void register_null_printer() {
    tiny_logger::PrinterRegistry::instance().register_printer(
        tiny_logger::PrinterType::Null,
        [](const tiny_logger::PrinterConfig& cfg) { return std::make_unique<tiny_logger::NullPrinter>(cfg); });
}
} // namespace tiny_logger