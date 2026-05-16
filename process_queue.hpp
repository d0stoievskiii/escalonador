#pragma once

#include <vector>
#include "processo.hpp"

class ProcessQueue
{
private:
    std::vector<Processo*> processos;
    uint32_t count;

public:
    ProcessQueue() {
        count = 0;
    }

    uint32_t size() {
        return count;
    }

    bool empty() {
        return processos.empty();
    }

    Processo* pop() {
        Processo* p = processos.front();
        processos.erase(processos.begin());
        count--;
        return p;
    }

    void push(Processo* p) {
        processos.emplace_back(p);
        count++;
    }
};