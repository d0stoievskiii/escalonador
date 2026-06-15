FROM gcc:14-bookworm AS build
WORKDIR /build
COPY main.cpp processo.cpp disco.cpp memoria.cpp \
     processo.hpp disco.hpp memoria.hpp cpu.hpp \
     sistema.hpp systemclock.hpp interrupt.hpp \
     process_queue.hpp escalonador_cp.hpp escalonador_lp.hpp escalonador_mp.hpp ./
RUN g++ main.cpp processo.cpp disco.cpp memoria.cpp -o simulador

FROM python:3.12-slim
WORKDIR /app
COPY --from=build /build/simulador ./
COPY main.py entrada.py monitor.py processo.py recurso.py ./
RUN pip install --no-cache-dir nicegui
EXPOSE 8080
CMD ["python", "main.py"]
