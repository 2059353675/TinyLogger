#include "TinyLogger/logger.h"
#include "TinyLogger/logger_error.h"

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

void Logger::init() {
    PrinterConfig console_cfg;
    console_cfg.type = PrinterType::Console;
    console_cfg.min_level = LogLevel::Info;

    LoggerConfig default_cfg;
    default_cfg.printers.emplace_back(console_cfg);

    init(default_cfg);
}

void Logger::init(const LoggerConfig& config) {
    if (distributor_) {
        throw std::logic_error("Logger already initialized");
    }

    if (config.buffer_size == 0) {
        throw make_invalid_buffer_size_error(config.buffer_size);
    }

    buffer_size_ = config.buffer_size;
    overflow_policy_ = config.overflow_policy;

    distributor_ = std::make_unique<Distributor>(registry_);

    for (const auto& pc : config.printers) {
        auto printer = PrinterRegistry::instance().create(pc);
        if (!printer) {
            throw make_printer_create_error(pc);
        }
        distributor_->add_printer(std::move(printer));
    }

    (void)get_queue();

    std::atomic_thread_fence(std::memory_order_release);

    distributor_->start();
}

void Logger::shutdown() {
    if (distributor_) {
        distributor_->stop();
        distributor_.reset();
    }
}

bool Logger::set_printer_min_level(PrinterType type, LogLevel level) {
    if (distributor_) {
        return distributor_->set_printer_min_level(type, level);
    }
    return false;
}

void Logger::handle_overflow() {
    switch (overflow_policy_) {
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
    auto q = std::make_unique<RingBuffer>(buffer_size_, overflow_policy_);
    RingBuffer* ptr = q.get();
    owned_queues_.push_back(std::move(q));
    registry_.register_queue(ptr);
    return ptr;
}

} // namespace tiny_logger