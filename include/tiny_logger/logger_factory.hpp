#pragma once

#include "logger.hpp"
#include <memory>

namespace tiny_logger {

std::unique_ptr<Logger> create_logger(const LoggerConfig& cfg);

} // namespace tiny_logger