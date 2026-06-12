#pragma once
 
#include "sistema.hpp"
#include "processo.hpp"
#include "process_queue.hpp"
#include "systemclock.hpp"
#include <iostream>
 
static constexpr uint32_t QUANTUM = 2;
 
class EscalonadorCP
{
private:
    Sistema& sistema;
 
    void log_estado(Processo* p, const std::string& de, const std::string& para) {
        std::cout << "[t=" << sistema.relogio.time() << "] "
                  << "Processo #" << p->get_pid()
                  << ": " << de << " -> " << para << "\n";
    }
 
    void log_despacho(Processo* p, uint32_t cpu_id) {
        std::cout << "[t=" << sistema.relogio.time() << "] "
                  << "Processo #" << p->get_pid()
                  << " despachado para CPU " << cpu_id << "\n";
    }
 
    Processo* proximo_pronto_user() {
        // Processos de Usuário precisam de garantir um disco
        for (int i = 0; i < 3; i++) {
            ProcessQueue& fila = sistema.user_ready_queues[i];
            
            // Espreitamos todos os processos na fila
            for (int j = 0; j < fila.size(); j++) {
                Processo* p = fila[j];
                
                // Se ainda não tem um disco associado, tentamos alocar um
                if (p->get_assigned_disk() == -1) {
                    int disco_livre = -1;
                    for (int k = 0; k < 4; k++) {
                        if (!sistema.discos[k].is_reserved()) {
                            disco_livre = k;
                            break;
                        }
                    }
                    
                    if (disco_livre != -1) {
                        // Sucesso! Tranca o disco, regista no processo e despacha
                        sistema.discos[disco_livre].reserve();
                        p->set_assigned_disk(disco_livre);
                        fila.remove(p->get_pid()); // Tira da fila
                        return p;
                    }
                    // Se não tem disco livre, salta este processo por enquanto e verifica o próximo
                } else {
                    // Já tem disco reservado (e.g., já rodou antes), pode ser despachado normalmente
                    fila.remove(p->get_pid());
                    return p;
                }
            }
        }
        return nullptr;
    }
 
    void despachar() {
        // 1. Prioridade Absoluta: Processos de Tempo Real (RT)
        while (!sistema.RT_ready_queue.empty()) {
            int cpu_alvo = -1;

            // Tenta encontrar uma CPU livre
            for (int i = 0; i < 4; i++) {
                if (sistema.processadores[i].ger_running_process() == nullptr) {
                    cpu_alvo = i;
                    break;
                }
            }

            // Se não há CPU livre, procura uma rodando um processo de Usuário (USER) para preemptar
            if (cpu_alvo == -1) {
                for (int i = 0; i < 4; i++) {
                    Processo* rodando = sistema.processadores[i].ger_running_process();
                    if (rodando != nullptr && rodando->get_prioridade() == USER) {
                        cpu_alvo = i;
                        break;
                    }
                }
            }

            // Se encontrou uma CPU (livre ou preemptada)
            if (cpu_alvo != -1) {
                Processo* rodando = sistema.processadores[cpu_alvo].ger_running_process();
                
                // Lógica de Preempção
                if (rodando != nullptr) {
                    sistema.processadores[cpu_alvo].remove_process();
                    rodando->set_state(PRONTO);
                    log_estado(rodando, "EXECUTANDO", "PRONTO (Preempcao RT)");
                    
                    // Devolve o usuário para a fila de onde veio (sem rebaixar, pois a culpa não foi do quantum)
                    sistema.user_ready_queues[rodando->get_queue_level()].push(rodando);
                }

                // Despacha o RT
                Processo* p_rt = sistema.RT_ready_queue.pop();
                p_rt->set_state(EXECUTANDO);
                log_estado(p_rt, "PRONTO", "EXECUTANDO");
                log_despacho(p_rt, cpu_alvo);
                
                // Manda rodar com quantum 0 (ininterrupto)
                sistema.processadores[cpu_alvo].run(p_rt, 0); 
            } else {
                // Todas as 4 CPUs estão rodando processos RT. Não há quem preemptar.
                break; 
            }
        }

        // 2. Preenche as CPUs que restaram livres com processos de Usuário
        for (int i = 0; i < 4; i++) {
            if (sistema.processadores[i].ger_running_process() != nullptr) continue; 
            
            Processo* p = proximo_pronto_user();
            if (p == nullptr) break; 
 
            p->set_state(EXECUTANDO);
            log_estado(p, "PRONTO", "EXECUTANDO");
            log_despacho(p, i);
 
            sistema.processadores[i].run(p, QUANTUM);
        }
    }
 
    void tick() {
        for (int i = 0; i < 4; i++) {
            ExecResult res = sistema.processadores[i].exec();
            
            // Se estiver ociosa ou rodando normalmente, não faz nada
            if (res == ExecResult::IDLE || res == ExecResult::RUNNING) continue;
 
            // Caso contrário, algo aconteceu e precisamos retirar o processo da CPU
            Processo* p = sistema.processadores[i].remove_process();
 
            if (res == ExecResult::FINISHED) {
                p->set_finish_time(sistema.relogio.time()); 
                sistema.finished_queue.push(p);
                log_estado(p, "EXECUTANDO", "FINALIZADO");

                sistema.mem.free_process(p);

                if (p->get_assigned_disk() != -1) {
                    sistema.discos[p->get_assigned_disk()].free_reservation();
                }
 
            } else if (res == ExecResult::BLOCKED) {
                sistema.blocked_queue.push(p);
                log_estado(p, "EXECUTANDO", "BLOQUEADO");
 
            } else if (res == ExecResult::QUANTUM_EXPIRED) {
                p->set_state(PRONTO);
                log_estado(p, "EXECUTANDO", "PRONTO");
                
                // Lógica do rebaixamento
                uint8_t lvl = p->get_queue_level();
                if (lvl < 2) {
                    p->set_queue_level(lvl + 1);
                }
                
                sistema.user_ready_queues[p->get_queue_level()].push(p);
                std::cout << "[t=" << sistema.relogio.time() << "] "
                          << "Processo #" << p->get_pid()
                          << " rebaixado para fila de usuario " << (int)p->get_queue_level() << "\n";
            }
        }
    }
 
public:
    explicit EscalonadorCP(Sistema& s) : sistema(s) {}
 
    void executar_tick() {
        tick();
        despachar();
    }
 
    void desbloquear(Processo* p) {
        sistema.blocked_queue.remove(p->get_pid());
        p->set_state(PRONTO);
        log_estado(p, "BLOQUEADO", "PRONTO");
        
        // Retorna para a fila que ele salvou na própria classe Processo
        sistema.user_ready_queues[p->get_queue_level()].push(p);
    }
 
    void imprimir_status() const {
        std::cout << "\n--- Status do Sistema [t=" << sistema.relogio.time() << "] ---\n";
        for (int i = 0; i < 4; i++) {
            Processo* p = sistema.processadores[i].ger_running_process();
            std::cout << "  CPU " << i << ": " << (p ? "Processo #" + std::to_string(p->get_pid()) : "livre") << "\n";
        }
        std::cout << "  Bloqueados    : " << sistema.blocked_queue.size()      << "\n";
        std::cout << "  Finalizados   : " << sistema.finished_queue.size()     << "\n\n";
    }
};