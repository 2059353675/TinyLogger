#include "printer.h"
#include "ring_buffer.h"
#include "types.h"
#include <thread>

namespace TinyLogger {

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

    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace TinyLogger