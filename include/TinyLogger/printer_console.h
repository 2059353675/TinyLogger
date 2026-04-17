#pragma once

#include "TinyLogger/printer.h"
#include <cstdio>

namespace tiny_logger {

class ConsolePrinter : public Printer
{
public:
    explicit ConsolePrinter(const PrinterConfig& config);

    void write(const LogEvent& event) override;

    void flush() override;
};

void register_console_printer();

} // namespace tiny_logger