#include "types.h"

namespace TinyLogger {

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