#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "sistema.hpp"
#include "escalonador_lp.hpp"
#include "escalonador_mp.hpp"
#include "escalonador_cp.hpp"

// Função para ler o ficheiro de texto e popular a fila de processos novos
bool carregar_processos_do_arquivo(const std::string& nome_arquivo, ProcessQueue& fila_novos) {
    std::ifstream arquivo(nome_arquivo);
    
    if (!arquivo.is_open()) {
        std::cerr << "Erro: Não foi possível abrir o ficheiro " << nome_arquivo << "\n";
        return false;
    }

    std::string linha;
    int processos_carregados = 0;

    while (std::getline(arquivo, linha)) {
        std::stringstream ss(linha);
        char ch;
        
        // Percorre a linha caractere a caractere, ignorando os espaços em branco
        while (ss >> ch) {
            if (ch == '[') {
                uint32_t id, cpu1, io, cpu2;
                size_t ram;
                char v1, v2, v3, v4, fecha_parenteses;

                if (ss >> id >> v1 >> cpu1 >> v2 >> io >> v3 >> cpu2 >> v4 >> ram >> fecha_parenteses) {
                    
                    if (v1 == ',' && v2 == ',' && v3 == ',' && v4 == ',' && fecha_parenteses == ']') {
                        
                        Processo* novo_proc = new Processo(id, cpu1, io, cpu2, ram);
                        
                        // VALIDAÇÃO DA REGRA DE TEMPO REAL: Max 512 MB
                        if (novo_proc->get_prioridade() == REALTIME && ram > 512) {
                            std::cerr << "Erro: Processo RT #" << id << " excede o limite de 512 MB.\n";
                            delete novo_proc;
                            continue; // Ignora este processo e passa ao próximo
                        }

                        fila_novos.push(novo_proc);
                        processos_carregados++;
                    }
                }
            }
        }
    }
    
    arquivo.close();
    std::cout << "Foram carregados " << processos_carregados << " processos com sucesso!\n";
    return true;
}

int main(int argc, char* argv[]) {
    std::string ficheiro_entrada = "processos.txt"; // Valor padrão

    if (argc > 1) {
        ficheiro_entrada = argv[1]; // Obtém o nome do arquivo de entrada da linha de comando
    }
    
    // 1. Instanciar o hardware simulado
    Sistema sistema;

    // 2. Tentar carregar os processos do ficheiro
    if (!carregar_processos_do_arquivo(ficheiro_entrada, sistema.novo_queue)) {
        return 1; // Sai se não encontrar o ficheiro
    }

    // 3. Inicializar os escalonadores
    ProcessQueue* ponteiros_user_queues[3] = {
        &sistema.user_ready_queues[0], 
        &sistema.user_ready_queues[1], 
        &sistema.user_ready_queues[2]
    };

    EscalonadorMP escalonador_medio_prazo(&sistema.RT_ready_queue, ponteiros_user_queues, 
                                          &sistema.ready_suspended_queue, &sistema.blocked_queue, 
                                          &sistema.blocked_suspended_queue);
    
    EscalonadorLP escalonador_longo_prazo(&sistema.novo_queue, &sistema.RT_ready_queue, ponteiros_user_queues, &escalonador_medio_prazo);
    
    EscalonadorCP escalonador_curto_prazo(sistema);

    // Guardamos o total de processos carregados para saber quando parar
    int total_processos = sistema.novo_queue.size();

    std::cout << "\n--- Iniciando a Simulacao ---\n";

    // Motor de Ticks
    while (sistema.finished_queue.size() < total_processos) {
        
        // 1. Escalonador de Longo Prazo (Admissão)
        // Usamos um while para tentar admitir o máximo de processos possível neste tick 
        // até que a fila esvazie ou a RAM encha.
        while (escalonador_longo_prazo.init_next_process()) {
            // Processo entrou na RAM e foi para a fila de prontos
        }

        // 2. Gestão de E/S (Discos)
        // a) Distribuir processos bloqueados que ainda não foram para os discos
        for (int i = 0; i < sistema.blocked_queue.size(); i++) {
            Processo* p = sistema.blocked_queue[i];
            
            if (!p->get_io_started()) {
                int disco_escolhido = p->get_assigned_disk();
                // Apenas por segurança, garantimos que tem um disco alocado
                if (disco_escolhido != -1) {
                    sistema.discos[disco_escolhido].request_io(*p, p->get_io_time(), sistema.relogio.time());
                    p->set_io_started(true);
                }
            }
        }

        // b) Avançar o tempo dos discos e verificar quem terminou
        for (int i = 0; i < 4; i++) {
            IOInterrupt* interrupcao = sistema.discos[i].process_io();
            
            if (interrupcao != nullptr) {
                // O disco gerou uma interrupção, o I/O terminou!
                uint32_t pid_desbloquear = interrupcao->get_pid();
                
                // Procurar o processo na fila de bloqueados
                int idx = sistema.blocked_queue.index(pid_desbloquear);
                if (idx != -1) {
                    Processo* p = sistema.blocked_queue[idx];
                    p->set_io_started(false); // Reinicia a variável
                    
                    // O escalonador remove-o da blocked_queue e devolve-o às user_ready_queues
                    escalonador_curto_prazo.desbloquear(p);
                }
                
                // Libertar a memória alocada dinamicamente pelo disco para evitar leaks
                delete interrupcao; 
            }
        }

        // 3. Escalonador de Curto Prazo (Execução e Despacho)
        // Avança o tempo de CPU dos processos que estão rodando e lida com
        // interrupções de quantum, finalizações e bloqueios.
        escalonador_curto_prazo.executar_tick();

        // 4. Escalonador de Médio Prazo (Swapping)
        escalonador_medio_prazo.swap_in();

        // 5. Avançar o Relógio do Sistema
        SystemClock::get()++;
        
        escalonador_curto_prazo.imprimir_status();
    }

    std::cout << "\n--- Simulacao Concluida ---\n";
    std::cout << "Tempo total decorrido: " << SystemClock::get().time() << " u.t.\n";

    std::cout << "\n--- Metricas Finais ---\n";
    std::cout << "PID\tChegada\tFim\tServico\tTurnaround\tTurnaround Normalizado\n";
    
    while (!sistema.finished_queue.empty()) {
        Processo* p = sistema.finished_queue.pop();
        Metrics m = p->generate_metrics();
        
        std::cout << m.pid << "\t" 
                  << m.arrival << "\t" 
                  << m.end << "\t" 
                  << m.service_time << "\t" 
                  << m.turnaround << "\t\t" 
                  << m.normalized_turnaround << "\n";
                  
        // Limpa a alocação dinâmica
        delete p;
    }

    return 0;
}