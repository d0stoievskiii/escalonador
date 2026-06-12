#include "memoria.hpp"
#include <stdexcept>
#include <algorithm>

uint32_t RAM::get_free_ram(uint32_t start) {
    uint32_t count = 0;
    for(int i = start; i < SIZE; i++) {
        if (physical_mem[i] == 0) count++;
    }
    return count;
}

int RAM::_find_free_block(size_t size) {
    uint32_t pos = 0;
    int start = pos;
    size_t length = 0;
    while (pos < SIZE) {
        if (physical_mem[pos] == 0) {
            length++;
            if (length >= size) {
                return start;
            }
        } else {
            start = pos + 1;
            length = 0;
        }
        pos++;
    }
    return -1;//not found
}

bool RAM::alloc(Processo* p) {
    Imagem ret;
    uint32_t size = p->get_size();
    int start_add = _find_free_block(size);
    if (start_add >= 0) {
        ret.pid = p->get_pid();
        ret.start_address = start_add;
        ret.end_address = start_add + size;

        memset(physical_mem+start_add, ret.pid, size);

        processos.push_back(ret);

        return true;
    }
    return false;
}

Imagem& RAM::_image_by_pid(uint32_t pid) {
    for (auto& p : processos) {
        if (p.pid == pid) {
            return p;
        }
    }
    throw std::runtime_error("Process not in memory");
}

void RAM::free_process(Processo *p) {
    uint32_t pid = p->get_pid();
    
    for (auto it = processos.begin(); it != processos.end(); ++it) {
        if (it->pid == pid) {
            memset(physical_mem + it->start_address, 0, p->get_size());
            processos.erase(it);
            break;
        }
    }
}