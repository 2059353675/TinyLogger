#include "tiny_logger/logger_factory.hpp"
#include "tiny_logger/logger.hpp"

namespace tiny_logger {

std::unique_ptr<Logger> create_logger(const LoggerConfig& cfg) {
    auto logger = std::make_unique<Logger>();
    logger->init(cfg);
    return logger;
}

} // namespace tiny_logger