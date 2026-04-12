#pragma once

#include "types.h"

namespace TinyLogger {

inline std::string format_timestamp(uint64_t ts_us) {
    using namespace std::chrono;

    system_clock::time_point tp{microseconds(ts_us)};
    std::time_t t = system_clock::to_time_t(tp);

    std::tm tm{};
    localtime_r(&t, &tm);

    auto us = ts_us % 1000000;

    return fmt::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:06}", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
                       tm.tm_min, tm.tm_sec, us);
}

inline const char* level_to_string(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "Debug";
    case LogLevel::Info:
        return "Info";
    case LogLevel::Error:
        return "Error";
    case LogLevel::Fatal:
        return "Fatal";
    default:
        return "Unknown";
    }
}

class Printer
{
public:
    virtual ~Printer() = default;

    virtual void init(const json& cfg) = 0;
    virtual void write(const LogEvent& event) = 0;
    virtual void flush() = 0;

    bool should_log(LogLevel lvl) const {
        return static_cast<uint8_t>(lvl) >= static_cast<uint8_t>(min_level_);
    }

    void set_level(LogLevel lvl) {
        min_level_ = lvl;
    }

protected:
    LogLevel min_level_;
};

} // namespace TinyLogger