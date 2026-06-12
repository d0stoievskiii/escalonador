#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "sistema.hpp"
#include "escalonador_lp.hpp"
#include "escalonador_mp.hpp"
#include "escalonador_cp.cpp"

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

int main() {
    // 1. Instanciar o hardware simulado
    Sistema sistema;

    // 2. Tentar carregar os processos do ficheiro
    std::string ficheiro_entrada = "processos.txt";
    if (!carregar_processos_do_arquivo(ficheiro_entrada, sistema.novo_queue)) {
        return 1; // Sai se não encontrar o ficheiro
    }

    // 3. Inicializar os escalonadores
    ProcessQueue* ponteiros_user_queues[3] = {
        &sistema.user_ready_queues[0], 
        &sistema.user_ready_queues[1], 
        &sistema.user_ready_queues[2]
    };

    EscalonadorLP escalonador_longo_prazo(&sistema.novo_queue, &sistema.RT_ready_queue, ponteiros_user_queues);
    
    EscalonadorMP escalonador_medio_prazo(&sistema.RT_ready_queue, ponteiros_user_queues, 
                                          &sistema.ready_suspended_queue, &sistema.blocked_queue, 
                                          &sistema.blocked_suspended_queue);
    
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

        // 2. Gestão de E/S (Discos) - [Feature 2]
        // TODO: Percorrer os 4 discos, verificar se terminaram o E/S,
        // gerar a interrupção e chamar escalonador_curto_prazo.desbloquear(p)

        // 3. Escalonador de Curto Prazo (Execução e Despacho)
        // Avança o tempo de CPU dos processos que estão rodando e lida com
        // interrupções de quantum, finalizações e bloqueios.
        escalonador_curto_prazo.executar_tick();

        // 4. Escalonador de Médio Prazo (Swapping) - [Feature 4]
        // TODO: Se houver processos suspensos e RAM livre, ou RAM cheia e processos precisando entrar.

        // 5. Avançar o Relógio do Sistema
        SystemClock::get()++;
        
        escalonador_curto_prazo.imprimir_status();
    }

    std::cout << "\n--- Simulacao Concluida ---\n";
    std::cout << "Tempo total decorrido: " << SystemClock::get().time() << " u.t.\n";

    // Chamar um método para iterar sobre a finished_queue e imprimir as métricas (Turnaround, etc)

    return 0;
}