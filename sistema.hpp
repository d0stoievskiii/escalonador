#pragma once

#include "cpu.hpp"
#include "disco.hpp"
#include "memoria.hpp"
#include "process_queue.hpp"
#include "systemclock.hpp"

class Sistema
{
private:
    SystemClock& relogio;
    RAM mem;
    Disco discos[4];
    CPU processadores[4];

    //new
    ProcessQueue novo_queue;

    //ready queues
    ProcessQueue RT_ready_queue;
    ProcessQueue user_ready_queues[3];
    ProcessQueue ready_suspended_queue;

    //blocked queues
    ProcessQueue blocked_queue;
    ProcessQueue blocked_suspended_queue;

    //finished
    ProcessQueue finished_queue;

public:
    Sistema() : relogio(SystemClock::get()),
                mem(),
                discos{ Disco(0), Disco(1), Disco(2), Disco(3) },
                processadores{ CPU(0), CPU(1), CPU(2), CPU(3) },
                novo_queue(),
                RT_ready_queue(),
                user_ready_queues{ ProcessQueue(), ProcessQueue(), ProcessQueue() },
                ready_suspended_queue(),
                blocked_queue(),
                blocked_suspended_queue(),
                finished_queue() {}



};