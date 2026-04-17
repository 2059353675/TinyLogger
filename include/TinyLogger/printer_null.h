#pragma once

#include "TinyLogger/printer.h"
#include <cstdio>

namespace tiny_logger {

class NullPrinter : public Printer
{
public:
    explicit NullPrinter(const PrinterConfig& config);

    void write(const LogEvent& event) override;

    void flush() override;
};

void register_null_printer();

} // namespace tiny_logger