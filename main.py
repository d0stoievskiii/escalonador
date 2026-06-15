import asyncio
import os
from nicegui import ui

from entrada import GerenciadorEntrada
from monitor import MonitorSistema

ui.dark_mode().enable()

# Remove as setas +/- nativas do navegador em todos os inputs numericos.
ui.add_css('''
    input[type=number]::-webkit-inner-spin-button,
    input[type=number]::-webkit-outer-spin-button { -webkit-appearance: none; margin: 0; }
    input[type=number] { -moz-appearance: textfield; }
''')

gerenciador_entrada = GerenciadorEntrada()
monitor_sistema     = MonitorSistema()

_CMD_COMPILACAO = ['g++', 'main.cpp', 'processo.cpp', 'disco.cpp', 'memoria.cpp', '-o', 'simulador']

async def _confirmar_compilacao():
    """Exibe modal perguntando se o usuario deseja compilar o backend. Retorna True se confirmado."""
    with ui.dialog() as dialog, ui.card().classes('bg-slate-800 border border-slate-600 p-6 min-w-[400px]'):
        ui.label('Binario nao encontrado').classes('text-lg font-bold text-amber-400 mb-2')
        ui.label('./simulador nao encontrado. Deseja compilar agora?').classes('text-sm text-slate-300')
        ui.label('Comando de compilacao:').classes('text-xs text-slate-500 mt-3')
        ui.code(' '.join(_CMD_COMPILACAO)).classes('text-xs bg-slate-900 rounded p-2 w-full mt-1')
        with ui.row().classes('mt-5 gap-3 justify-end w-full'):
            ui.button('Cancelar', on_click=lambda: dialog.submit(False)).classes('bg-slate-600')
            ui.button('Compilar', on_click=lambda: dialog.submit(True)).classes('bg-amber-600 font-bold')
    return await dialog

async def _compilar_binario():
    """Executa o comando g++. Retorna True em sucesso; exibe dialog de erro em falha."""
    ui.notify('Compilando...', type='info')

    proc = await asyncio.create_subprocess_exec(
        *_CMD_COMPILACAO,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )
    _, stderr = await proc.communicate()

    if proc.returncode == 0:
        ui.notify('Compilacao concluida com sucesso!', type='positive')
        return True

    texto_erro = stderr.decode('utf-8').strip()
    with ui.dialog() as dialog_erro, ui.card().classes('bg-slate-800 border border-red-800 p-6 min-w-[500px] max-w-[90vw]'):
        ui.label('Falha na compilacao').classes('text-lg font-bold text-red-400 mb-2')
        ui.code(texto_erro).classes('text-xs bg-slate-900 rounded p-3 w-full overflow-auto max-h-[50vh]')
        ui.button('Fechar', on_click=dialog_erro.close).classes('mt-4 bg-slate-600')
    dialog_erro.open()
    return False

async def iniciar_simulacao():
    if not gerenciador_entrada.formatar_e_salvar():
        return

    if not os.path.exists('./simulador'):
        if not await _confirmar_compilacao():
            return
        if not await _compilar_binario():
            return

    ui.notify('Simulacao iniciada...', type='info')
    monitor_sistema.limpar()

    processo = await asyncio.create_subprocess_exec(
        './simulador',
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )

    while True:
        linha = await processo.stdout.readline()
        if not linha:
            break
        linha_decodificada = linha.decode('utf-8').rstrip()

        monitor_sistema.processar_linha_log(linha_decodificada)

        atraso = 1.0 - gerenciador_entrada.controle_velocidade.value
        if atraso > 0:
            await asyncio.sleep(atraso)

    await processo.wait()

    monitor_sistema.renderizar_painel()
    ui.notify('Simulacao concluida!', type='positive')

# --- Orquestracao da UI ---
with ui.row().classes('w-full max-w-[98vw] mx-auto mt-4 gap-6 flex-nowrap items-start'):

    with ui.column().classes('w-[580px] shrink-0 bg-slate-900 border border-slate-700 p-4 rounded'):
        gerenciador_entrada.construir_ui()
        ui.button('Iniciar Simulação', on_click=iniciar_simulacao).classes(
            'w-full bg-amber-600 font-bold text-lg h-12 mt-6'
        )

    with ui.column().classes('flex-grow w-full min-w-0 bg-slate-900 border border-slate-700 p-4 rounded'):
        monitor_sistema.construir_ui()

ui.run(host='0.0.0.0', port=8080, show=False)
