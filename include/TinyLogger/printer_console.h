#pragma once

#include "TinyLogger/printer.h"
#include <cstdio>

namespace TinyLogger {
class ConsolePrinter : public Printer
{
public:
    void init(const json& cfg) override {
        // stdout 默认即可
    }

    void write(const LogEvent& event) override {
        std::fwrite(event.buffer, 1, event.length, stdout);
        if (event.buffer[event.length - 1] != '\n') {
            std::fwrite("\n", 1, 1, stdout);
        }
    }

    void flush() override {
        std::fflush(stdout);
    }
};
} // namespace TinyLogger