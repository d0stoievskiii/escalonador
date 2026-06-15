from nicegui import ui
from processo import renderizar_gantts_separados
from recurso import renderizar_gantt_unificado

class MonitorSistema:
    def __init__(self):
        self.tick_atual = 0
        self.tick_maximo = 0
        self.historico = {
            'CPU 0': {}, 'CPU 1': {}, 'CPU 2': {}, 'CPU 3': {},
            'Disco 0': {}, 'Disco 1': {}, 'Disco 2': {}, 'Disco 3': {}
        }
        self.dados_metricas = []
        self.lendo_metricas = False

        # E/S de disco: {pid_str: set de ticks de status em que o processo estava em E/S}
        self.historico_es = {}
        self._em_es = {}

        # Fila de prontos por tick: {tick: {'rt': [...], 'u0': [...], 'u1': [...], 'u2': [...]}}
        self.historico_fila = {}
        self._fila_pronta = []          # [{'pid', 'nivel', 'is_rt', 'ordem'}]
        self._fila_suspensa_pronta = set()
        self._nivel_fila = {}           # pid -> nivel atual na fila de usuario
        self._pids_rt = set()
        self._ordem_insercao = 0

        self.mostrar_es   = True
        self.mostrar_fila = False
        self.ja_renderizado = False

        self.exp_separado  = None
        self.exp_unificado = None
        self.exp_metricas  = None
        self.exp_console   = None

        self.container_separado  = None
        self.container_unificado = None
        self.container_metricas  = None
        self.log_view = None

    def construir_ui(self):
        ui.label('Monitor do Sistema').classes('text-2xl font-bold text-slate-200 mb-2')

        with ui.expansion('Trace de CPUs Individuais', icon='query_stats').classes(
            'w-full bg-slate-800 text-slate-200 border border-slate-700 mb-2'
        ) as self.exp_separado:
            ui.switch('Mostrar E/S de Disco', value=True, on_change=self._toggle_es).classes(
                'text-slate-300 mb-1'
            )
            self.container_separado = ui.column().classes('w-full')
            self.exp_separado.open()

        with ui.expansion('Trace Unificado', icon='timeline').classes(
            'w-full bg-slate-800 text-slate-200 border border-slate-700 mb-2'
        ) as self.exp_unificado:
            ui.switch('Mostrar Fila de Prontos', value=False, on_change=self._toggle_fila).classes(
                'text-slate-300 mb-1'
            )
            self.container_unificado = ui.column().classes('w-full')

        with ui.expansion('Métricas', icon='table_chart').classes(
            'w-full bg-slate-800 text-slate-200 border border-slate-700 mb-2'
        ) as self.exp_metricas:
            self.container_metricas = ui.column().classes('w-full')

        with ui.expansion('Console', icon='terminal').classes(
            'w-full bg-slate-800 text-slate-400 border border-slate-700'
        ) as self.exp_console:
            self.log_view = ui.log().classes('w-full h-[300px] bg-black text-green-400 font-mono text-xs p-2')
            self.exp_console.open()

    def _re_renderizar_separado(self):
        self.container_separado.clear()
        renderizar_gantts_separados(
            self.container_separado,
            self.historico,
            self.tick_maximo,
            self.historico_es if self.mostrar_es else None,
        )

    def _re_renderizar_unificado(self):
        self.container_unificado.clear()
        renderizar_gantt_unificado(
            self.container_unificado,
            self.historico,
            self.tick_maximo,
            self.historico_fila,
            mostrar_fila=self.mostrar_fila,
        )

    def _toggle_es(self, e):
        self.mostrar_es = e.value
        if self.ja_renderizado:
            self._re_renderizar_separado()

    def _toggle_fila(self, e):
        self.mostrar_fila = e.value
        if self.ja_renderizado:
            self._re_renderizar_unificado()

    def limpar(self):
        self.log_view.clear()
        self.tick_atual = 0
        self.tick_maximo = 0
        self.dados_metricas.clear()
        self.lendo_metricas = False
        for chave in self.historico:
            self.historico[chave].clear()

        self.container_separado.clear()
        self.container_unificado.clear()
        self.container_metricas.clear()

        self.historico_es.clear()
        self._em_es.clear()

        self.historico_fila.clear()
        self._fila_pronta.clear()
        self._fila_suspensa_pronta.clear()
        self._nivel_fila.clear()
        self._pids_rt.clear()
        self._ordem_insercao = 0
        self.ja_renderizado = False

        self.exp_console.open()

    def processar_linha_log(self, linha: str):
        self.log_view.push(linha)

        # --- Status do sistema: snapshot da fila por nivel ---
        if 'Status do Sistema [t=' in linha:
            self.tick_atual = int(linha.split('[t=')[1].split(']')[0])
            self.tick_maximo = max(self.tick_maximo, self.tick_atual)
            fila_ord = sorted(
                self._fila_pronta,
                key=lambda x: (-x['is_rt'], x['nivel'], x['ordem'])
            )
            snapshot = {'rt': [], 'u0': [], 'u1': [], 'u2': []}
            for p in fila_ord:
                if p['is_rt']:
                    snapshot['rt'].append(p['pid'])
                else:
                    key = f"u{p['nivel']}"
                    if key in snapshot:
                        snapshot[key].append(p['pid'])
            self.historico_fila[self.tick_atual] = snapshot
            return

        # --- CPU status (impresso por imprimir_status) ---
        if 'CPU ' in linha and ':' in linha:
            partes = linha.split(':')
            nome_cpu = partes[0].strip()
            status   = partes[1].strip()
            if 'Processo #' in status:
                pid = status.replace('Processo #', '')
                if nome_cpu in self.historico:
                    self.historico[nome_cpu][self.tick_atual] = pid
            return

        # --- Disco status (impresso por imprimir_status) ---
        if 'Disco ' in linha and ':' in linha:
            partes = linha.split(':')
            nome_disco = partes[0].strip()
            status     = partes[1].strip()
            if 'Processo #' in status:
                pid = status.replace('Processo #', '')
                if nome_disco in self.historico:
                    self.historico[nome_disco][self.tick_atual] = pid
            return

        # --- Inicio de E/S ---
        if 'EXECUTANDO -> BLOQUEADO' in linha:
            t_log = int(linha.split('[t=')[1].split(']')[0])
            pid   = linha.split('Processo #')[1].split(':')[0].strip()
            self._em_es[pid] = t_log
            return

        # --- Fim de E/S: registra ticks em disco e volta para fila ---
        if 'BLOQUEADO -> PRONTO' in linha:
            t_log = int(linha.split('[t=')[1].split(']')[0])
            pid   = linha.split('Processo #')[1].split(':')[0].strip()
            if pid in self._em_es:
                inicio = self._em_es.pop(pid)
                self.historico_es.setdefault(pid, set()).update(range(inicio + 1, t_log + 1))
            is_rt = pid in self._pids_rt
            self._fila_pronta.append({
                'pid': pid,
                'nivel': self._nivel_fila.get(pid, 0) if not is_rt else -1,
                'is_rt': is_rt,
                'ordem': self._ordem_insercao,
            })
            self._ordem_insercao += 1
            return

        # --- Admissao: detecta tipo do processo ---
        if 'Criando Processo #' in linha:
            pid = linha.split('Processo #')[1].split(' ')[0].strip()
            if 'REALTIME' in linha:
                self._pids_rt.add(pid)
            return

        # --- Admissao: processo entra na fila de prontos ---
        if ': NOVO -> PRONTO' in linha:
            pid = linha.split('Processo #')[1].split(':')[0].strip()
            is_rt = pid in self._pids_rt
            self._fila_pronta.append({
                'pid': pid,
                'nivel': -1 if is_rt else 0,
                'is_rt': is_rt,
                'ordem': self._ordem_insercao,
            })
            self._ordem_insercao += 1
            return

        # --- Despacho: processo sai da fila de prontos ---
        if ': PRONTO -> EXECUTANDO' in linha:
            pid = linha.split('Processo #')[1].split(':')[0].strip()
            self._fila_pronta = [p for p in self._fila_pronta if p['pid'] != pid]
            return

        # --- Preempcao ou quantum: processo volta para a fila de prontos ---
        if ': EXECUTANDO -> PRONTO' in linha:
            pid = linha.split('Processo #')[1].split(':')[0].strip()
            is_rt = pid in self._pids_rt
            self._fila_pronta.append({
                'pid': pid,
                'nivel': self._nivel_fila.get(pid, 0) if not is_rt else -1,
                'is_rt': is_rt,
                'ordem': self._ordem_insercao,
            })
            self._ordem_insercao += 1
            return

        # --- Rebaixamento: atualiza nivel do processo na fila ---
        if 'rebaixado para fila de usuario' in linha:
            pid = linha.split('Processo #')[1].split(' ')[0].strip()
            novo_nivel = int(linha.split('fila de usuario ')[1].strip())
            self._nivel_fila[pid] = novo_nivel
            for p in self._fila_pronta:
                if p['pid'] == pid:
                    p['nivel'] = novo_nivel
                    break
            return

        # --- Swap Out (Pronto): processo suspenso sai da fila de prontos ---
        if 'Swapping Out Processo #' in linha and '(Pronto Fila' in linha:
            pid = linha.split('Processo #')[1].split(' ')[0].strip()
            self._fila_pronta = [p for p in self._fila_pronta if p['pid'] != pid]
            self._fila_suspensa_pronta.add(pid)
            return

        # --- Swap In: processo pronto-suspenso volta para a fila ---
        if 'retornou para a RAM' in linha:
            pid = linha.split('Processo #')[1].split(' ')[0].strip()
            if pid in self._fila_suspensa_pronta:
                self._fila_suspensa_pronta.discard(pid)
                is_rt = pid in self._pids_rt
                self._fila_pronta.append({
                    'pid': pid,
                    'nivel': self._nivel_fila.get(pid, 0) if not is_rt else -1,
                    'is_rt': is_rt,
                    'ordem': self._ordem_insercao,
                })
                self._ordem_insercao += 1
            return

        # --- Metricas finais ---
        if 'PID\tChegada\tFim\tServico\tTurnaround' in linha:
            self.lendo_metricas = True
            return

        if self.lendo_metricas and linha.strip() != '':
            partes = linha.split()
            if len(partes) >= 6 and partes[0].isdigit():
                self.dados_metricas.append({
                    'pid': partes[0], 'chegada': partes[1], 'fim': partes[2],
                    'servico': partes[3], 'turnaround': partes[4], 'norm_turnaround': partes[5]
                })

    def renderizar_painel(self):
        self.exp_console.close()
        self.ja_renderizado = True
        self._re_renderizar_separado()
        self._re_renderizar_unificado()
        self._renderizar_metricas()

    def _renderizar_metricas(self):
        colunas = [
            {'name': 'pid',            'label': 'PID',          'field': 'pid',            'align': 'left'},
            {'name': 'chegada',        'label': 'Chegada',      'field': 'chegada'},
            {'name': 'fim',            'label': 'Fim',          'field': 'fim'},
            {'name': 'servico',        'label': 'Servico',      'field': 'servico'},
            {'name': 'turnaround',     'label': 'Turnaround',   'field': 'turnaround'},
            {'name': 'norm_turnaround','label': 'Turnaround N.','field': 'norm_turnaround'},
        ]
        with self.container_metricas:
            if self.dados_metricas:
                ui.table(
                    columns=colunas, rows=self.dados_metricas, row_key='pid'
                ).classes('w-full bg-slate-800 text-slate-300 rounded')
            else:
                ui.label('Nenhum dado de metricas recebido.').classes('text-slate-500 text-sm p-4')
        self.exp_metricas.open()
