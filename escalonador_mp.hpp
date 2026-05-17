#pragma once

#include <stdexcept>

#include "memoria.hpp"
#include "systemclock.hpp"
#include "process_queue.hpp"


class EscalonadorMP
{
private:
    ProcessQueue* RT_ready_queue;
    ProcessQueue** user_ready_queues;
    ProcessQueue* ready_suspended_queue;

    ProcessQueue* blocked_queue;
    ProcessQueue* blocked_suspended_queue;

public:
    EscalonadorMP(ProcessQueue* rt_q, ProcessQueue** u_q, ProcessQueue* r_s_q,
         ProcessQueue* b_q, ProcessQueue* b_s_q) : RT_ready_queue(rt_q),
         user_ready_queues(u_q), ready_suspended_queue(r_s_q), blocked_queue(b_q), blocked_suspended_queue(b_s_q) {}


    void _suspend(Processo* p) {
        ProcessState state = p->get_state();
        if (state != ProcessState::PRONTO && state != ProcessState::BLOQUEADO) {
            throw std::runtime_error("Estado de processo inesperado! [" + StateFlagsToString(state) + 
                    "] processID: [" + std::to_string(p->get_pid()) + "] @ EscalonadorMP._suspend");
        }
        state = ProcessState(state | ProcessState::SUSPENSO);
        p->set_state(state);
        p->img.start_address = -1;
        p->img.end_address = -1;

        uint32_t pid = p->get_pid();
        bool removed = false;
        if (state & ProcessState::PRONTO) {
            for (int i = 0; i < 4; i++) {
                if (user_ready_queues[i]->remove(pid)) {
                    removed = true;
                    break;
                }
            }

            if (!removed) {
                removed = RT_ready_queue->remove(pid);
            }

            ready_suspended_queue->push(p);
        } else {
            removed = blocked_queue->remove(pid);

            blocked_suspended_queue->push(p);
        }

        if (!removed) {//melhorar mensagem
            throw std::runtime_error("Processo não encontrado na lista esperada! processo: " 
                + std::to_string(p->get_pid()));
        }
    }

    bool unsuspend(Processo* p) {
        ProcessState state = p->get_state();
        if (!(state & ProcessState::SUSPENSO)) {
            throw std::runtime_error("Estado de processo inesperado! [" + StateFlagsToString(state) + 
                    "] processID: [" + std::to_string(p->get_pid()) + "] @ EscalonadorMP.unsuspend");
        }
        RAM& ram = RAM::get();
        
        state = ProcessState(state & ~(ProcessState::SUSPENSO));
        if (ram.alloc(p)) {
            p->set_state(state);
            p->img = ram._image_by_pid(p->get_pid());

            if (state & ProcessState::PRONTO) {
                if (p->get_prioridade() ==  Prioridade::USER) {
                    user_ready_queues[0]->push(p);
                } else {
                    RT_ready_queue->push(p);
                }
            } else if (state & ProcessState::BLOQUEADO) {
                blocked_queue->push(p);
            } else {
                throw std::runtime_error("Estado de processo inesperado! [" + StateFlagsToString(state) + 
                    "] processID: [" + std::to_string(p->get_pid()) + "] @ EscalonadorMP.unsuspend2");
            }
            return true;
        }
        return false;
    }

    bool swap_out(size_t size) {
        /*
        EscalonadorLP quer dar CP pra um processo novo mas nao encontra espaco, chama essa funcao, que decide quem tirar
        da MP. Se não conseguir resolver o problema retorna falso
        */
       return true;
    }

};