#pragma once

#include <cstdint>
#include <string>

enum Prioridade : uint32_t
{
    REALTIME,
    USER
};

enum ProcessState : uint32_t
{
    NOVO = 1 << 0,//1
    PRONTO = 1 << 1,//2
    BLOQUEADO = 1 << 2,//4...
    SUSPENSO = 1 << 3,
    EXECUTANDO = 1 << 4,
    FINALIZADO = 1 << 5
};
//pronto suspenso = pronto + suspenso = 2 + 8 = 10
//bloqueado suspenso = bloqueado + suspenso = 4 + 8 = 12

typedef struct ProcessStateAndString {
    ProcessState value;
    std::string_view name;
};

constexpr ProcessStateAndString kProcessStateTable[6] =
{
	{ NOVO, "Novo"},
	{ PRONTO,    "Pronto" },
	{ BLOQUEADO, "Bloqueado" },
	{ SUSPENSO, "Suspenso" },
	{ EXECUTANDO, "Executando" },
	{ FINALIZADO, "Finalizado" }
};

std::string StateFlagsToString(uint32_t flags);

typedef struct PROCESS_IMAGE {
    uint32_t pid;
    int32_t start_address;
    int32_t end_address;
} Imagem;

typedef struct PROCESS_METRICS {
    uint32_t pid;
    uint32_t arrival;
    uint32_t end;
    uint32_t service_time;
    uint32_t turnaround;
    float normalized_turnaround;
} Metrics;

class Processo 
{
private:
    uint32_t pid;
    uint32_t cpu_time_1;
    uint32_t cpu_time_2;
    uint32_t service_time;
    uint32_t io_time;
    uint32_t arrival_time;
    uint32_t finish_time;
    uint32_t exec_phase;
    
    size_t mbs_ram;

    ProcessState estado;
    Prioridade prioridade;

    uint8_t queue_level; // Rastreia a fila de feedback (0, 1 ou 2)
    
public:

    Imagem img;
    
    Processo(uint32_t processid, uint32_t cpu_t_1,
    uint32_t cpu_t_2, uint32_t io_t, size_t ram) {
        pid = processid;
        cpu_time_1 = cpu_t_1;
        cpu_time_2 = cpu_t_2;
        io_time = io_t;
        mbs_ram = ram;
        
        service_time = cpu_t_1 + cpu_t_2;
        //imagem ainda não existe na memoria
        img.pid = pid;
        img.start_address = -1;
        img.end_address = -1;
        //estado é novo até o escalonador LP entrar em ação
        estado = NOVO;
        exec_phase = 1;

        queue_level = 0;
        
        //pela descrição do trabalho minha interpretação é que se diferencia REALTIME de USER pelo tempo de IO
        //REALTIME nunca tem, user sempre tem(perguntar pra ela)
        if (io_time == 0) {
            prioridade = REALTIME;
        } else {
            prioridade = USER;
        }
    }

    Metrics generate_metrics();

    Prioridade get_prioridade();

    ProcessState get_state();
    ProcessState set_state(ProcessState s);

    uint32_t time_left();

    void _block_or_end();

    bool exec();

    uint32_t get_pid();

    size_t get_size();

    uint32_t set_arrival_time(uint32_t time);

    uint32_t set_finish_time(uint32_t time);

    uint8_t get_queue_level() const { return queue_level; }

    void set_queue_level(uint8_t lvl) { queue_level = lvl; }
};