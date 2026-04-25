#pragma once

#include "tiny_logger/config.h"
#include "tiny_logger/logger.h"
#include <memory>

namespace tiny_logger {

std::unique_ptr<Logger> create_logger(const LoggerConfig& cfg);

} // namespace tiny_logger