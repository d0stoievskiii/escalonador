from nicegui import ui

_CORES = [
    ('#2563eb', '#fff'),  # azul
    ('#059669', '#fff'),  # verde
    ('#d97706', '#fff'),  # amarelo
    ('#7c3aed', '#fff'),  # roxo
    ('#e11d48', '#fff'),  # vermelho
    ('#0891b2', '#fff'),  # ciano
]

def _cor_por_pid(pid):
    return _CORES[int(pid) % len(_CORES)]

def _rgba(hex_color, alpha):
    h = hex_color.lstrip('#')
    r, g, b = int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
    return f'rgba({r},{g},{b},{alpha})'


def renderizar_gantts_separados(container, historico, tick_maximo, historico_es=None):
    _fixo = (
        'position:sticky;left:0;z-index:2;'
        'background:#0f172a;border:1px solid #334155;'
        'padding:4px 10px;color:#e2e8f0;font-size:11px;'
        'font-weight:bold;text-align:left;white-space:nowrap;'
    )
    _cabecalho = (
        'border:1px solid #334155;padding:2px 6px;'
        'color:#64748b;font-size:10px;min-width:30px;text-align:center;'
    )
    _vazio = (
        'border:1px solid #1e293b;background:#1e293b;'
        'height:28px;min-width:30px;'
    )

    cabecalhos_tick = ''.join(
        f'<th style="{_cabecalho}">{t}</th>' for t in range(tick_maximo + 1)
    )

    with container:
        alguma_ativa = False
        for cpu_id in range(4):
            chave_cpu = f'CPU {cpu_id}'
            pids_ativos = sorted(set(historico.get(chave_cpu, {}).values()), key=int)
            if not pids_ativos:
                continue
            alguma_ativa = True

            linhas_html = ''
            for pid in pids_ativos:
                bg, fg = _cor_por_pid(pid)
                cor_es = _rgba(bg, 0.4)
                celulas = ''
                for t in range(tick_maximo + 1):
                    if historico[chave_cpu].get(t) == pid:
                        celulas += (
                            f'<td style="border:1px solid #334155;background:{bg};color:{fg};'
                            f'font-size:10px;font-weight:bold;text-align:center;'
                            f'height:28px;min-width:30px;padding:0 4px;">{pid}</td>'
                        )
                    elif historico_es and t in historico_es.get(pid, set()):
                        celulas += (
                            f'<td style="{_vazio}box-shadow:inset 0 0 0 2px {cor_es};'
                            f'text-align:center;vertical-align:middle;cursor:default;"'
                            f' title="Processo #{pid} em E/S de disco">'
                            f'<span class="material-icons" '
                            f'style="font-size:14px;color:{cor_es};line-height:28px;">storage</span>'
                            '</td>'
                        )
                    else:
                        celulas += f'<td style="{_vazio}"></td>'
                linhas_html += f'<tr><th style="{_fixo}">Processo #{pid}</th>{celulas}</tr>'

            html = (
                '<div style="overflow-x:auto;width:100%;border:1px solid #334155;border-radius:4px;">'
                '<table style="border-collapse:collapse;white-space:nowrap;width:max-content;">'
                f'<thead><tr><th style="{_fixo}"></th>{cabecalhos_tick}</tr></thead>'
                f'<tbody>{linhas_html}</tbody>'
                '</table></div>'
            )

            ui.label(f'CPU {cpu_id}').classes('text-sm font-bold text-slate-400 mt-4 mb-1')
            ui.html(html).classes('w-full')

        if not alguma_ativa:
            ui.label('Nenhuma atividade de CPU registrada.').classes('text-slate-500 text-sm p-4')
