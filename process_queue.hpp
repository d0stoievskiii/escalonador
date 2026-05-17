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

    int index(uint32_t pid) {
        int id = 0;
        for (const auto p : processos) {
            if (p->get_pid() == pid) {
                return id;
            }
            id++;
        }
        return -1;
    }

    Processo* operator[](int index) {
        return processos[index];
    }

    bool remove(uint32_t pid) {
        int id = index(pid);
        if (id == -1) {
            return false;
        }

        processos.erase(processos.begin() + id);
        return true;
    }


};