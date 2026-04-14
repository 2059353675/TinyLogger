#include "TinyLogger/printer_null.h"

namespace TinyLogger {

NullPrinter::NullPrinter(const PrinterConfig& config) {
    min_level_ = config.min_level;
}

void NullPrinter::write(const LogEvent& event) {
}

void NullPrinter::flush() {
}

void register_null_printer() {
    TinyLogger::PrinterRegistry::instance().register_printer(
        TinyLogger::PrinterType::Null,
        [](const TinyLogger::PrinterConfig& cfg) { return std::make_unique<TinyLogger::NullPrinter>(cfg); });
}
} // namespace TinyLogger