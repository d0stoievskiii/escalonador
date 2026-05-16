#pragma once

#include <stdexcept>
#include "interrupt.hpp"

class CPU
{
private:
    uint32_t current_time;
    uint32_t id;
    Processo* executing;

public:
    CPU(uint32_t i) {
        id = i;
        executing = nullptr;
    }

    void run(Processo* p) {
        if (executing == nullptr) {
            executing = p;
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
    /*
    avança o tempo atual, executa o processo atual, se houver
    retorna verdadeiro se CPU está ocupada, falso se está desocupada
    */
    bool exec() {
        current_time++;
        if (executing != nullptr) {
            if (!executing->exec()) {
                executing = nullptr;
                return false;
            }
            return true;
        }
        return false;
    }
};