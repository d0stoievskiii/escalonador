from nicegui import ui

class GerenciadorEntrada:
    def __init__(self):
        self.lista_processos = []
        self._proximo_id = 0  # chave interna de linha, independente do PID

        self.tabela = None
        self.novo_pid = None
        self.novo_cpu1 = None
        self.novo_es = None
        self.novo_cpu2 = None
        self.novo_ram = None
        self.controle_velocidade = None

        self.carga_padrao = [
            {'pid': 100, 'cpu1': 15, 'es': 0,  'cpu2': 0,  'ram': 256},
            {'pid': 101, 'cpu1': 8,  'es': 0,  'cpu2': 0,  'ram': 512},
            {'pid': 1,   'cpu1': 6,  'es': 4,  'cpu2': 2,  'ram': 5000},
            {'pid': 2,   'cpu1': 2,  'es': 10, 'cpu2': 2,  'ram': 5000},
            {'pid': 3,   'cpu1': 12, 'es': 2,  'cpu2': 10, 'ram': 5000},
            {'pid': 4,   'cpu1': 4,  'es': 4,  'cpu2': 4,  'ram': 18000},
            {'pid': 5,   'cpu1': 4,  'es': 2,  'cpu2': 4,  'ram': 2000},
            {'pid': 6,   'cpu1': 2,  'es': 2,  'cpu2': 2,  'ram': 1000},
        ]

    # ------------------------------------------------------------------ auxiliares

    def _criar_linha(self, pid, cpu1, es, cpu2, ram):
        linha = {'_id': self._proximo_id, 'pid': pid, 'cpu1': cpu1, 'es': es, 'cpu2': cpu2, 'ram': ram}
        self._proximo_id += 1
        return linha

    def _atualizar_tabela(self):
        self.tabela.rows = self.lista_processos[:]
        self.tabela.update()

    def _pid_existe(self, pid, excluir_id=None):
        return any(p['pid'] == pid and p['_id'] != excluir_id for p in self.lista_processos)

    def _proximo_pid_livre(self, a_partir_de):
        ocupados = {p['pid'] for p in self.lista_processos}
        pid = a_partir_de
        while pid in ocupados:
            pid += 1
        return pid

    # --------------------------------------------------------------- mutacoes

    def carregar_padrao(self):
        self.lista_processos.clear()
        self._proximo_id = 0
        for p in self.carga_padrao:
            self.lista_processos.append(
                self._criar_linha(p['pid'], p['cpu1'], p['es'], p['cpu2'], p['ram'])
            )
        self._atualizar_tabela()
        ui.notify('Exemplo Carregado', type='positive')

    def limpar_tabela(self):
        self.lista_processos.clear()
        self._proximo_id = 0
        self._atualizar_tabela()

    def adicionar_processo(self):
        try:
            pid = int(self.novo_pid.value)
        except TypeError:
            ui.notify('Todos os campos devem ser numericos', type='negative')
            return

        if self._pid_existe(pid):
            ui.notify(f'PID {pid} ja existe na tabela', type='warning')
            return

        try:
            linha = self._criar_linha(
                pid,
                int(self.novo_cpu1.value),
                int(self.novo_es.value),
                int(self.novo_cpu2.value),
                int(self.novo_ram.value),
            )
        except TypeError:
            ui.notify('Todos os campos devem ser numericos', type='negative')
            return

        self.lista_processos.append(linha)
        self._atualizar_tabela()
        self.novo_pid.value = self.novo_cpu1.value = self.novo_es.value = \
            self.novo_cpu2.value = self.novo_ram.value = None

    # ----------------------------------------------------------- handlers de slot

    def _ao_atualizar_linha(self, e):
        atualizado = e.args
        novo_pid = atualizado['pid']
        id_linha  = atualizado['_id']

        if self._pid_existe(novo_pid, excluir_id=id_linha):
            ui.notify(f'PID {novo_pid} ja existe — use um ID unico', type='warning')

        for p in self.lista_processos:
            if p['_id'] == id_linha:
                p.update(atualizado)
                break
        # Sem _atualizar_tabela() — o Vue ja reflete a edicao; re-renderizar
        # reiniciaria a posicao do cursor enquanto o usuario ainda edita.

    def _ao_deletar(self, e):
        rid = int(e.args)
        self.lista_processos[:] = [p for p in self.lista_processos if p['_id'] != rid]
        self._atualizar_tabela()

    def _ao_duplicar(self, e):
        rid = int(e.args)
        idx = next((i for i, p in enumerate(self.lista_processos) if p['_id'] == rid), None)
        if idx is None:
            return
        orig    = self.lista_processos[idx]
        novo_pid = self._proximo_pid_livre(orig['pid'] + 1)
        nova_linha = self._criar_linha(novo_pid, orig['cpu1'], orig['es'], orig['cpu2'], orig['ram'])
        self.lista_processos.insert(idx + 1, nova_linha)
        self._atualizar_tabela()

    # ------------------------------------------------------------ saida para arquivo

    def formatar_e_salvar(self):
        if not self.lista_processos:
            ui.notify('Tabela vazia. Adicione processos primeiro.', type='warning')
            return False

        pids = [p['pid'] for p in self.lista_processos]
        if len(pids) != len(set(pids)):
            duplicados = sorted({pid for pid in pids if pids.count(pid) > 1})
            ui.notify(f'PIDs duplicados: {duplicados} — corrija antes de executar', type='negative')
            return False

        with open('processos.txt', 'w') as f:
            for p in self.lista_processos:
                f.write(f"[{p['pid']}, {p['cpu1']}, {p['es']}, {p['cpu2']}, {p['ram']}]\n")
        return True

    # -------------------------------------------------------------- layout da UI

    def construir_ui(self):
        colunas = [
            {'name': 'pid',     'label': 'PID',      'field': 'pid',  'align': 'left'},
            {'name': 'cpu1',    'label': 'CPU Fase 1','field': 'cpu1', 'align': 'center'},
            {'name': 'es',      'label': 'E/S',       'field': 'es',   'align': 'center'},
            {'name': 'cpu2',    'label': 'CPU Fase 2','field': 'cpu2', 'align': 'center'},
            {'name': 'ram',     'label': 'RAM (MB)',  'field': 'ram',  'align': 'center'},
            {'name': 'acoes',   'label': '',          'field': 'acoes'},
        ]

        with ui.column().classes('w-full gap-0'):
            ui.label('Matriz de Controle de Processos').classes('text-2xl font-bold text-slate-200 mb-3')

            with ui.row().classes('w-full gap-3 mb-3'):
                ui.button('Carregar Exemplo', on_click=self.carregar_padrao).classes('bg-blue-600')
                ui.button('Limpar Processos',   on_click=self.limpar_tabela).classes('bg-red-600')

            ui.separator().classes('bg-slate-700 mb-3')

            ui.label('Adicionar Processo').classes('text-base font-bold text-slate-300 mb-2')
            with ui.row().classes('w-full gap-2 items-end flex-nowrap'):
                self.novo_pid  = ui.number('PID',    format='%.0f').classes('w-[65px]')
                self.novo_cpu1 = ui.number('CPU 1',  format='%.0f').classes('w-[80px]')
                self.novo_es   = ui.number('E/S',    format='%.0f').classes('w-[70px]')
                self.novo_cpu2 = ui.number('CPU 2',  format='%.0f').classes('w-[80px]')
                self.novo_ram  = ui.number('RAM',    format='%.0f').classes('w-[90px]')
                ui.button('Adicionar', on_click=self.adicionar_processo).classes('bg-green-600 h-[56px] px-4')

            ui.separator().classes('bg-slate-700 mt-3 mb-3')

            ui.label('Velocidade da Animacao').classes('text-sm font-bold text-slate-400')
            with ui.row().classes('w-full items-center gap-4 mb-3'):
                self.controle_velocidade = ui.slider(min=0.0, max=1.0, step=0.01, value=1.0).classes('flex-grow')
                ui.label().bind_text_from(
                    self.controle_velocidade, 'value',
                    backward=lambda v: 'Instantaneo' if v >= 1.0 else f'{int((1.0 - v) * 1000)} ms/tick'
                ).classes('text-slate-300 font-mono text-sm w-[100px] text-right')

            ui.separator().classes('bg-slate-700 mb-3')

            self.tabela = ui.table(
                columns=colunas, rows=self.lista_processos, row_key='_id'
            ).classes('w-full bg-slate-800 text-slate-300')

            self.tabela.add_slot('body', r'''
                <q-tr :props="props">
                    <q-td key="pid" :props="props">
                        <q-input v-model.number="props.row.pid" type="number" dense borderless
                            input-class="text-slate-200 text-left"
                            @blur="() => $parent.$emit('linhaAtualizada', {...props.row})" />
                    </q-td>
                    <q-td key="cpu1" :props="props">
                        <q-input v-model.number="props.row.cpu1" type="number" dense borderless
                            input-class="text-slate-200 text-center"
                            @blur="() => $parent.$emit('linhaAtualizada', {...props.row})" />
                    </q-td>
                    <q-td key="es" :props="props">
                        <q-input v-model.number="props.row.es" type="number" dense borderless
                            input-class="text-slate-200 text-center"
                            @blur="() => $parent.$emit('linhaAtualizada', {...props.row})" />
                    </q-td>
                    <q-td key="cpu2" :props="props">
                        <q-input v-model.number="props.row.cpu2" type="number" dense borderless
                            input-class="text-slate-200 text-center"
                            @blur="() => $parent.$emit('linhaAtualizada', {...props.row})" />
                    </q-td>
                    <q-td key="ram" :props="props">
                        <q-input v-model.number="props.row.ram" type="number" dense borderless
                            input-class="text-slate-200 text-center"
                            @blur="() => $parent.$emit('linhaAtualizada', {...props.row})" />
                    </q-td>
                    <q-td key="acoes" :props="props" auto-width>
                        <div class="row no-wrap">
                            <q-btn flat dense round icon="content_copy" color="blue-4" size="sm"
                                @click="() => $parent.$emit('duplicarLinha', props.row._id)" />
                            <q-btn flat dense round icon="delete" color="red-5" size="sm"
                                @click="() => $parent.$emit('deletarLinha', props.row._id)" />
                        </div>
                    </q-td>
                </q-tr>
            ''')

            self.tabela.on('linhaAtualizada', self._ao_atualizar_linha)
            self.tabela.on('deletarLinha',    self._ao_deletar)
            self.tabela.on('duplicarLinha',   self._ao_duplicar)
