#pragma once

#include <stdexcept>
#include <iostream>

#include "memoria.hpp"
#include "systemclock.hpp"
#include "process_queue.hpp"
#include "escalonador_mp.hpp"

class EscalonadorLP
{
private:
    ProcessQueue* novo_queue;

    ProcessQueue* RT_ready_queue;
    ProcessQueue** user_ready_queues;
    EscalonadorMP* mp;

public:
    EscalonadorLP(ProcessQueue* nov, ProcessQueue* rt_q, ProcessQueue** u_q, EscalonadorMP* medio_p) : 
        novo_queue(nov), RT_ready_queue(rt_q), user_ready_queues(u_q), mp(medio_p) {}

    bool init_next_process() {
        if (!novo_queue->empty()) {
            Processo *p = novo_queue->pop(); 
            RAM& ram = RAM::get();

            // Tenta alocar. Se falhar, pede o swap_out e tenta de novo!
            if (ram.alloc(p) || (mp->swap_out(p->get_size()) && ram.alloc(p))) {
                
                ProcessState estado = p->get_state();
                if (estado != ProcessState::NOVO) {
                    throw std::runtime_error("Estado de processo inesperado!");
                }

                std::cout << "[t=" << SystemClock::get().time() << "] Criando Processo #" << p->get_pid() 
                          << " (RAM: " << p->get_size() << " MB, Tipo: " 
                          << (p->get_prioridade() == Prioridade::REALTIME ? "REALTIME" : "USER") << ")\n";
                std::cout << "[t=" << SystemClock::get().time() << "] Processo #" << p->get_pid() << ": NOVO -> PRONTO\n";

                p->set_state(ProcessState::PRONTO);
                p->img = ram._image_by_pid(p->get_pid());
                p->set_arrival_time(SystemClock::get().time());

                if (p->get_prioridade() ==  Prioridade::USER) {
                    user_ready_queues[0]->push(p);
                } else {
                    RT_ready_queue->push(p);
                }
                return true;
            }
            
            // Falhou de vez (ex: nenhum processo vítima disponível)
            novo_queue->push(p);
        }
        return false;
    }
};