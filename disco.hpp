#pragma once

#include <queue>
#include "cpu.hpp"
#include "systemclock.hpp"


typedef struct IO_REQUEST {
    uint32_t process_id;
    uint32_t start_time;
    uint32_t end_time;
};

class IOInterrupt : public Interrupt
{
private:
    IO_REQUEST request;

public:
    IOInterrupt(uint32_t time, IO_REQUEST r) : Interrupt(time) {
        request = r;
    }

    InterruptReason reason() override {
        return InterruptReason::IO;
    }

};

class Disco
{
private:
    std::queue<IO_REQUEST> io_queue;
    IO_REQUEST processing;
    bool busy;
    uint32_t id;

public:
    Disco(uint32_t num_disco) {
        id = num_disco;
        busy = false;
    }

    IO_REQUEST request_io(Processo& p, uint32_t duration, uint32_t start);

    IOInterrupt* process_io();
    bool isAvailable();

};