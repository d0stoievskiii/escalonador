#pragma once

#include <stdexcept>
#include <iostream>

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

        RAM::get().free_process(p);

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

            // CORREÇÃO: Tirar da fila de suspensos antes de devolver à fila ativa
            if (state & ProcessState::PRONTO) {
                ready_suspended_queue->remove(p->get_pid());
                
                if (p->get_prioridade() == Prioridade::USER) {
                    user_ready_queues[p->get_queue_level()]->push(p);
                } else {
                    RT_ready_queue->push(p);
                }
            } else if (state & ProcessState::BLOQUEADO) {
                blocked_suspended_queue->remove(p->get_pid());
                blocked_queue->push(p);
            } else {
                throw std::runtime_error("Estado de processo inesperado! [" + StateFlagsToString(state) + 
                    "] processID: [" + std::to_string(p->get_pid()) + "] @ EscalonadorMP.unsuspend2");
            }
            std::cout << "[t=" << SystemClock::get().time() << "] Processo #" << p->get_pid() << " retornou para a RAM (Swap In)\n";
            return true;
        }
        return false;
    }

    // Retira processos até haver um bloco contíguo de memória suficiente
    bool swap_out(size_t size) {
        RAM& ram = RAM::get();
        
        // Vítima nível 1: Processos bloqueados
        while (ram._find_free_block(size) == -1 && !blocked_queue->empty()) {
            Processo* victim = (*blocked_queue)[0];
            std::cout << "[t=" << SystemClock::get().time() << "] Swapping Out Processo #" << victim->get_pid() << " (Bloqueado)\n";
            _suspend(victim);
        }

        // Vítima nível 2: Processos de utilizador prontos (da prioridade menor para a maior)
        for (int i = 2; i >= 0; i--) {
            while (ram._find_free_block(size) == -1 && !user_ready_queues[i]->empty()) {
                Processo* victim = (*user_ready_queues[i])[0];
                std::cout << "[t=" << SystemClock::get().time() << "] Swapping Out Processo #" << victim->get_pid() << " (Pronto Fila " << i << ")\n";
                _suspend(victim);
            }
        }
        
        // Verifica se conseguiu libertar espaço suficiente
        return ram._find_free_block(size) != -1;
    }

    // Traz os processos de volta se houver espaço
    void swap_in() {
        // Tenta trazer os bloqueados suspensos primeiro
        for (int i = 0; i < blocked_suspended_queue->size(); i++) {
            Processo* p = (*blocked_suspended_queue)[i];
            if (unsuspend(p)) i--; // O vetor encolhe, ajustamos o índice
        }
        // Tenta trazer os prontos suspensos
        for (int i = 0; i < ready_suspended_queue->size(); i++) {
            Processo* p = (*ready_suspended_queue)[i];
            if (unsuspend(p)) i--; 
        }
    }
};