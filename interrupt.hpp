#pragma once

#include <cstdint>
#include "processo.hpp"

enum InterruptReason : uint8_t
{
    TIME,
    IO
};

class Interrupt
{
private:
    uint32_t time;

public:
    Interrupt(uint32_t system_time) {
        time = system_time;
    }

    virtual InterruptReason reason() = 0;

};

class TimeSliceInterrupt : public Interrupt
{
private:
    Processo &processo;

public:
    TimeSliceInterrupt(uint32_t time, Processo& p) : Interrupt(time), processo(p) {}

    InterruptReason reason() override {
        return InterruptReason::TIME;
    }

};
