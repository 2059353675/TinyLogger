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
        std::string ts = format_timestamp(event.timestamp);

        std::string line = fmt::format("[{}][{}][{}] {}", ts, event.thread_id, level_to_string(event.level),
                                       std::string_view(event.buffer, event.length));

        std::fwrite(line.data(), 1, line.size(), stdout);

        if (line.empty() || line.back() != '\n') {
            std::fwrite("\n", 1, 1, stdout);
        }
    }

    void flush() override {
        std::fflush(stdout);
    }
};
} // namespace TinyLogger