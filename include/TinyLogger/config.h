#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace tiny_logger {

enum class ErrorCode {
    InvalidBufferSize,
    InvalidOverflowPolicy,
    InvalidPrinterType,
    InvalidLevel,
    BufferAllocFailed,
    PrinterCreateFailed,
    UnknownError,
    None
};

} // namespace tiny_logger