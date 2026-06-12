#pragma once

#include "processo.hpp"
#include "systemclock.hpp"
#include <vector>
#include <cstring>

#define SIZE 32000

typedef unsigned int MB;

class RAM : public Singleton<RAM>
{
private:
    MB physical_mem[SIZE];
    std::vector<Imagem> processos;

public:
    RAM() {
        memset(physical_mem, 0, SIZE);  
    }
    uint32_t get_free_ram(uint32_t start=0);
    int _find_free_block(size_t size);
    bool alloc(Processo* p);
    Imagem& _image_by_pid(uint32_t pid);
    void free_process(Processo* p);

};