#pragma once

#include <stdexcept>

#include "memoria.hpp"
#include "systemclock.hpp"
#include "process_queue.hpp"

class EscalonadorLP
{
private:
    ProcessQueue* novo_queue;

    ProcessQueue* RT_ready_queue;
    ProcessQueue** user_ready_queues;
public:
    EscalonadorLP(ProcessQueue* nov, ProcessQueue* rt_q, ProcessQueue** u_q) : novo_queue(nov),
                                                                                RT_ready_queue(rt_q),
                                                                                user_ready_queues(u_q) {}

    bool init_next_process() {
        if (!novo_queue->empty()) {
            Processo *p = novo_queue->pop(); 
            RAM& ram = RAM::get();

            if (ram.alloc(p)) {
                ProcessState estado = p->get_state();
                if (estado != ProcessState::NOVO) {
                    throw std::runtime_error("Estado de processo inesperado! [" + StateFlagsToString(estado) + 
                    "] processID: [" + std::to_string(p->get_pid()) + "]");
                }
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
            /*
                codigo pedindo pro escalonador de medio prazo fazer o trabalho dele
            */
            //alocação falhou, e ai? volta pro inicio ou vai pro final da fila?
            novo_queue->push(p);
            return false;
        }
        
        return false;
    }
    

};