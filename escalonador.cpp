#pragma once
 
#include "sistema.hpp"
#include "processo.hpp"
#include "process_queue.hpp"
#include "systemclock.hpp"
 
/*
 * EscalonadorCP — Escalonador de Curto Prazo (Despachante)
 *
 * Responsabilidades:
 *  1. A cada tick do relógio, verifica CPUs ocupadas:
 *       - Se o processo terminou (cpu.exec() == false):
 *           * FINALIZADO   → move para finished_queue
 *           * BLOQUEADO    → move para blocked_queue (aguarda I/O)
 *  2. Trata interrupção por quantum (processos USER):
 *       - Conta ticks do processo na CPU
 *       - Ao atingir QUANTUM (2), remove da CPU e rebaixa na fila de feedback
 *  3. Para cada CPU livre, despacha o próximo processo:
 *       - Prioridade absoluta: RT_ready_queue  (FCFS, sem preempção)
 *       - Senão: user_ready_queues[0], [1], [2]  (feedback multinível)
 *  4. Ao despachar, muda estado: PRONTO → EXECUTANDO
 *  5. Imprime mensagem sempre que um processo muda de estado
 *
 * Política de feedback para processos USER:
 *   - Fila 0 (maior prioridade): quantum = 2 ticks
 *   - Fila 1 (média prioridade): quantum = 2 ticks
 *   - Fila 2 (menor prioridade): quantum = 2 ticks  (round-robin permanente)
 *   Ao expirar quantum na fila N, o processo vai para a fila N+1.
 *   Na fila 2 (última), o processo volta para o final da própria fila 2.
 */
 
static constexpr uint32_t QUANTUM = 2;
 
class EscalonadorCP
{
private:
    Sistema& sistema;
 
    // Contador de ticks que cada CPU está executando o processo atual.
    // Índice = índice do processador (0..3).
    uint32_t quantum_counter[4] = {0, 0, 0, 0};
 
    // ─── helpers de log ───────────────────────────────────────────────
 
    void log_estado(Processo* p, const std::string& de, const std::string& para) {
        std::cout << "[t=" << sistema.relogio.get_time() << "] "
                  << "Processo #" << p->get_pid()
                  << ": " << de << " → " << para << "\n";
    }
 
    void log_despacho(Processo* p, uint32_t cpu_id) {
        std::cout << "[t=" << sistema.relogio.get_time() << "] "
                  << "Processo #" << p->get_pid()
                  << " despachado para CPU " << cpu_id << "\n";
    }
 
    // ─── escolhe próxima fila de prontos com processo disponível ──────
 
    /*
     * Retorna um par {processo, índice_da_fila_user (-1 se RT)}.
     * Prioridade: RT primeiro, depois fila 0, 1, 2.
     */
    std::pair<Processo*, int> proximo_pronto() {
        // 1. Tempo Real tem prioridade absoluta
        if (!sistema.RT_ready_queue.empty()) {
            return { sistema.RT_ready_queue.pop(), -1 };
        }
        // 2. Filas de feedback de usuário em ordem de prioridade
        for (int i = 0; i < 3; i++) {
            if (!sistema.user_ready_queues[i].empty()) {
                return { sistema.user_ready_queues[i].pop(), i };
            }
        }
        return { nullptr, -1 };
    }
 
    // ─── rebaixa processo na fila de feedback ────────────────────────
 
    void rebaixar_fila(Processo* p, int fila_atual) {
        int proxima = (fila_atual >= 2) ? 2 : fila_atual + 1;
        sistema.user_ready_queues[proxima].push(p);
        std::cout << "[t=" << sistema.relogio.get_time() << "] "
                  << "Processo #" << p->get_pid()
                  << " rebaixado para fila de usuário " << proxima << "\n";
    }
 
    // ─── despacha processos para CPUs livres ─────────────────────────
 
    void despachar() {
        for (int i = 0; i < 4; i++) {
            if (sistema.processadores[i].ger_running_process() != nullptr) {
                continue; // CPU ocupada
            }
 
            auto [p, fila] = proximo_pronto();
            if (p == nullptr) break; // sem processos prontos
 
            // Muda estado PRONTO → EXECUTANDO
            p->set_state(EXECUTANDO);
            log_estado(p, "PRONTO", "EXECUTANDO");
            log_despacho(p, i);
 
            sistema.processadores[i].run(p);
            quantum_counter[i] = 0;
 
            // RT: sinaliza na CPU que não deve sofrer preempção por quantum
            // (tratado no passo de tick abaixo — processos RT nunca têm
            //  quantum expirado pois chegamos aqui só quando a CPU está livre)
        }
    }
 
    // ─── tick: avança CPUs e trata interrupções ──────────────────────
 
    void tick() {
        for (int i = 0; i < 4; i++) {
            Processo* p = sistema.processadores[i].ger_running_process();
            if (p == nullptr) continue;
 
            bool ainda_executando = sistema.processadores[i].exec();
            // exec() já decrementou cpu_time dentro de Processo::exec()
 
            if (!ainda_executando) {
                // O processo mudou de estado dentro de Processo::_block_or_end()
                ProcessState estado = p->get_state();
                sistema.processadores[i].remove_process();
                quantum_counter[i] = 0;
 
                if (estado == FINALIZADO) {
                    p->set_arrival_time(sistema.relogio.get_time()); // reaproveitamos como finish_time se necessário
                    sistema.finished_queue.push(p);
                    log_estado(p, "EXECUTANDO", "FINALIZADO");
 
                } else if (estado == BLOQUEADO) {
                    sistema.blocked_queue.push(p);
                    log_estado(p, "EXECUTANDO", "BLOQUEADO");
                    // O evento de I/O deverá ser tratado por outro componente
                    // que, ao concluir, moverá o processo de volta para a fila
                    // de prontos adequada e chamará desbloquear().
                }
 
            } else {
                // Processo ainda executando — verifica quantum (só para USER)
                if (p->get_prioridade() == USER) {
                    quantum_counter[i]++;
 
                    if (quantum_counter[i] >= QUANTUM) {
                        // Interrupção de relógio: quantum expirado
                        int fila_atual = obter_fila_atual(p);
                        sistema.processadores[i].remove_process();
                        quantum_counter[i] = 0;
 
                        p->set_state(PRONTO);
                        log_estado(p, "EXECUTANDO", "PRONTO");
                        rebaixar_fila(p, fila_atual);
                    }
                }
                // Processos RT nunca sofrem preempção por quantum (FCFS até conclusão)
            }
        }
    }
 
    /*
     * Determina em qual fila de usuário um processo estava antes de
     * ser despachado.
     *
     * Limitação: após o pop() na fila, essa informação se perde.
     * A solução correta é guardar o índice da fila no próprio Processo
     * (campo extra) ou no mapa interno do escalonador. Aqui usamos um
     * mapa simples pid → fila.
     */
    std::unordered_map<uint32_t, int> mapa_fila_pid;
 
    int obter_fila_atual(Processo* p) {
        auto it = mapa_fila_pid.find(p->get_pid());
        if (it != mapa_fila_pid.end()) return it->second;
        return 0; // padrão: fila 0
    }
 
public:
    explicit EscalonadorCP(Sistema& s) : sistema(s) {}
 
    // ─── interface principal ──────────────────────────────────────────
 
    /*
     * Chamado a cada tick do relógio do sistema.
     * Ordem:
     *   1. Avança CPUs e trata fim de quantum / fim de processo.
     *   2. Despacha processos prontos para CPUs que ficaram livres.
     */
    void executar_tick() {
        tick();
        despachar();
    }
 
    /*
     * Chamado pelo componente de I/O quando um processo termina
     * sua operação de disco e volta a ser PRONTO.
     *
     * O processo deve estar na blocked_queue.
     * Após desbloquear, vai para a fila de usuário adequada.
     */
    void desbloquear(Processo* p) {
        sistema.blocked_queue.remove(p->get_pid());
 
        p->set_state(PRONTO);
        log_estado(p, "BLOQUEADO", "PRONTO");
 
        // Volta para a fila do nível que estava (sem rebaixamento adicional)
        int fila = obter_fila_atual(p);
        sistema.user_ready_queues[fila].push(p);
    }
 
    /*
     * Chamado pelo EscalonadorLP quando um processo saiu de NOVO
     * e entrou em PRONTO (já está na fila certa do Sistema).
     * Aqui apenas registramos o índice da fila para uso futuro.
     *
     *  fila == -1  → processo RT (sem controle de quantum)
     *  fila == 0,1,2 → fila de usuário
     */
    void registrar_fila(Processo* p, int fila) {
        mapa_fila_pid[p->get_pid()] = fila;
    }
 
    // ─── debug / status ──────────────────────────────────────────────
 
    void imprimir_status() const {
        std::cout << "\n─── Status do Sistema [t="
                  << sistema.relogio.get_time() << "] ───\n";
        for (int i = 0; i < 4; i++) {
            Processo* p = sistema.processadores[i].ger_running_process();
            std::cout << "  CPU " << i << ": ";
            if (p) std::cout << "Processo #" << p->get_pid()
                             << " (quantum=" << quantum_counter[i] << ")";
            else   std::cout << "livre";
            std::cout << "\n";
        }
        std::cout << "  Prontos RT    : " << sistema.RT_ready_queue.size()     << "\n";
        std::cout << "  Prontos User0 : " << sistema.user_ready_queues[0].size() << "\n";
        std::cout << "  Prontos User1 : " << sistema.user_ready_queues[1].size() << "\n";
        std::cout << "  Prontos User2 : " << sistema.user_ready_queues[2].size() << "\n";
        std::cout << "  Bloqueados    : " << sistema.blocked_queue.size()      << "\n";
        std::cout << "  Finalizados   : " << sistema.finished_queue.size()     << "\n\n";
    }
};