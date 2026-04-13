#include "printer.h"
#include "ring_buffer.h"
#include "types.h"
#include <thread>

namespace TinyLogger {

static constexpr size_t LOG_LEVEL_COUNT = 5;

class Distributor
{
public:
    explicit Distributor(RingBuffer& rb);
    ~Distributor();

public:
    void start();
    void stop();
    void add_printer(std::unique_ptr<Printer> p);

private:
    void run();
    void drain_remaining();
    void flush_all();

private:
    RingBuffer& ring_buffer_;
    std::vector<std::unique_ptr<Printer>> printers_;
    std::array<std::vector<Printer*>, LOG_LEVEL_COUNT> level_routing_;

    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace TinyLogger