#include "disco.hpp"


IO_REQUEST Disco::request_io(Processo& p, uint32_t duration, uint32_t start) {
    IO_REQUEST req{p.get_pid(), start, start+duration};
    io_queue.push(req);

    return req;
}

bool Disco::isAvailable() {
    return !busy;
}

IOInterrupt* Disco::process_io() {
    if (isAvailable()) {
        if (!io_queue.empty()) {
            processing = io_queue.front();
            io_queue.pop();
            busy = !busy;
        }

        return nullptr;
    } else {
        uint32_t time = SystemClock::get().time();
        if (time < processing.end_time) {
            return nullptr;
        }
        IOInterrupt* Interrupt = new IOInterrupt(time, processing);
        busy = !busy;

        return Interrupt;
    }
}