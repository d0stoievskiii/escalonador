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
    EscalonadorLP escalonador_longo_prazo(&sistema.novo_queue, &sistema.RT_ready_queue, sistema.user_ready_queues);
    EscalonadorMP escalonador_medio_prazo(&sistema.RT_ready_queue, sistema.user_ready_queues, 
                                          &sistema.ready_suspended_queue, &sistema.blocked_queue, 
                                          &sistema.blocked_suspended_queue);
    EscalonadorCP escalonador_curto_prazo(sistema);

    // TODO: Iniciar o loop de simulação aqui (motor de ticks)

    return 0;
}