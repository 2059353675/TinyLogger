#include "TinyLogger/logger.h"

namespace tiny_logger {

namespace {
bool registered = [] {
    register_console_printer();
    register_file_printer();
    return true;
}();
} // namespace

std::atomic<uint64_t> Logger::next_instance_id_{1};

Logger::Logger() : instance_id_(next_instance_id_.fetch_add(1)) {
}

ErrorCode Logger::init() {
    PrinterConfig console_cfg;
    console_cfg.type = PrinterType::Console;
    console_cfg.min_level = LogLevel::Info;

    LoggerConfig default_cfg;
    default_cfg.printers.emplace_back(console_cfg);

    return init(default_cfg);
}

ErrorCode Logger::init(const std::string& path) {
    ErrorCode err;
    auto cfg = load_config(path, err);
    if (!cfg) {
        return err;
    }
    return init(*cfg);
}

ErrorCode Logger::init(const LoggerConfig& config) {
    config_ = config;

    distributor_ = std::make_unique<Distributor>(registry_);

    for (const auto& pc : config_->printers) {
        auto printer = PrinterRegistry::instance().create(pc);
        if (!printer) {
            return ErrorCode::PrinterCreateFailed;
        }
        distributor_->add_printer(std::move(printer));
    }

    (void)get_queue();

    distributor_->start();

    return ErrorCode::None;
}

void Logger::shutdown() {
    if (distributor_) {
        distributor_->stop();
        distributor_.reset();
    }
}

void Logger::set_overflow_policy(OverflowPolicy policy) {
    if (config_) {
        config_->overflow_policy = policy;
    }
}

bool Logger::set_printer_min_level(PrinterType type, LogLevel level) {
    if (distributor_) {
        return distributor_->set_printer_min_level(type, level);
    }
    return false;
}

OverflowPolicy Logger::get_overflow_policy() const {
    if (config_) {
        return config_->overflow_policy;
    }
    return OverflowPolicy::Discard;
}

void Logger::handle_overflow() {
    if (!config_) {
        return;
    }
    switch (config_->overflow_policy) {
        case OverflowPolicy::Discard:
            dropped_.fetch_add(1, std::memory_order_relaxed);
            break;
        case OverflowPolicy::Block:
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            break;
        default:
            break;
    }
}

RingBuffer* Logger::get_queue() {
    thread_local RingBuffer* queue = nullptr;
    thread_local uint64_t owner_id = 0;
    if (queue == nullptr || owner_id != instance_id_) {
        queue = create_and_register_queue();
        owner_id = instance_id_;
    }
    return queue;
}

RingBuffer* Logger::create_and_register_queue() {
    auto q = std::make_unique<RingBuffer>(config_->buffer_size);
    RingBuffer* ptr = q.get();
    owned_queues_.push_back(std::move(q));
    registry_.register_queue(ptr);
    return ptr;
}

} // namespace tiny_logger