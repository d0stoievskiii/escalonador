#pragma once

#include <stdexcept>
#include "interrupt.hpp"

enum class ExecResult { RUNNING, FINISHED, BLOCKED, QUANTUM_EXPIRED, IDLE };

class CPU
{
private:
    uint32_t current_time;
    uint32_t id;
    Processo* executing;
    uint32_t quantum_left;

public:
    CPU(uint32_t i) {
        id = i;
        executing = nullptr;
        quantum_left = 0;
        current_time = 0;
    }

    void run(Processo* p, uint32_t quantum = 0) {
        if (executing == nullptr) {
            executing = p;
            quantum_left = quantum;
        } else {
            throw std::runtime_error("CPU deveria estar vazia antes de receber um processo.");
        }
    }

    Processo* ger_running_process() {
        return executing;
    }

    Processo* remove_process() {
        Processo* ret = executing;
        executing = nullptr;
        return ret;
    }

    ExecResult exec() {
        current_time++;
        if (executing != nullptr) {
            bool ainda_executando = executing->exec();
            
            if (!ainda_executando) {
                ProcessState estado = executing->get_state();
                if (estado == ProcessState::FINALIZADO) return ExecResult::FINISHED;
                if (estado == ProcessState::BLOQUEADO) return ExecResult::BLOCKED;
            }

            if (quantum_left > 0) {
                quantum_left--;
                if (quantum_left == 0) {
                    return ExecResult::QUANTUM_EXPIRED;
                }
            }
            return ExecResult::RUNNING;
        }
        return ExecResult::IDLE;
    }
};