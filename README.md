# Simulador de Escalonador de Processos

Este projeto é um simulador completo de um **Gerenciador e Escalonador de Processos** para um Sistema Operacional com suporte a multiprocessamento (4 CPUs), gerenciamento de memória RAM com suporte a *Swapping*, exclusão mútua de recursos (4 Discos de E/S), e múltiplas políticas de escalonamento baseadas em níveis de prioridade.

O simulador foi desenvolvido de forma sequencial estruturada através de um loop principal guiado por um relógio lógico central (*System Clock Ticks*).

---

## Formato do Arquivo de Entrada

O simulador lê um arquivo de texto chamado `processos.txt`. Cada processo deve seguir a sintaxe exata:

```
[<id Processo>, <duração de CPU fase 1>, <Duração de I/O>, <duração de CPU fase 2>, <#Mbytes de RAM>]
```

## Como Compilar e Rodar (Com interface gráfica via Docker)

execute
```bash
docker run -p 8080:8080 tucoerthal/escalonador:latest
```
e depois acesse `http://localhost:8080/`

## Como Compilar e Rodar (Linha de comando apenas, Linux)

Certifique-se de ter o compilador g++ instalado com suporte a C++17 ou superior.

1. Navegue até o diretório raiz do projeto:

    ```
    cd /caminho/para/o/seu/projeto/escalonador
    ```

2. Certifique-se de que o arquivo processos.txt está presente na mesma pasta.

3. Compile todos os módulos do projeto rodando o comando unificado no terminal:

    ```
    g++ main.cpp processo.cpp disco.cpp memoria.cpp -o simulador
    ```


4. Execute o simulador gerado:

    ```
    ./simulador
    ```