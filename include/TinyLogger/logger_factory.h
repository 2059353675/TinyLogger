#pragma once

#include "TinyLogger/config.h"
#include "TinyLogger/logger.h"
#include <memory>

namespace tiny_logger {

std::unique_ptr<Logger> create_logger(const LoggerConfig& cfg);

} // namespace tiny_logger