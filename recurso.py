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


def renderizar_gantt_unificado(container, historico, tick_maximo, historico_fila=None, mostrar_fila=False):
    """Renderiza o rastro unificado por recurso como tabela HTML com coluna fixa."""
    recursos_ativos = [r for r in historico if historico[r]]

    _fixo = (
        'position:sticky;left:0;z-index:2;'
        'background:#0f172a;border:1px solid #334155;'
        'padding:4px 10px;color:#e2e8f0;font-size:11px;'
        'font-weight:bold;text-align:left;white-space:nowrap;'
    )
    _fixo_fila = (
        'position:sticky;left:0;z-index:2;'
        'background:#0f172a;border:1px solid #334155;'
        'padding:4px 10px;color:#94a3b8;font-size:11px;'
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

    linhas_html = ''
    for recurso in recursos_ativos:
        celulas = ''
        for t in range(tick_maximo + 1):
            pid = historico[recurso].get(t)
            if pid:
                bg, fg = _cor_por_pid(pid)
                celulas += (
                    f'<td style="border:1px solid #334155;background:{bg};color:{fg};'
                    f'font-size:10px;font-weight:bold;text-align:center;'
                    f'height:28px;min-width:30px;padding:0 4px;">{pid}</td>'
                )
            else:
                celulas += f'<td style="{_vazio}"></td>'
        linhas_html += f'<tr><th style="{_fixo}">{recurso}</th>{celulas}</tr>'

    cabecalhos_tick = ''.join(
        f'<th style="{_cabecalho}">{t}</th>' for t in range(tick_maximo + 1)
    )

    # Filas de prontos como <tfoot> — scroll sincronizado com os recursos
    tfoot_html = ''
    if historico_fila and mostrar_fila:
        _NIVEIS = [
            ('rt', 'Fila RT'),
            ('u0', 'Fila U0'),
            ('u1', 'Fila U1'),
            ('u2', 'Fila U2'),
        ]
        # So mostra niveis com atividade em pelo menos um tick
        niveis_ativos = [
            (nk, nl) for nk, nl in _NIVEIS
            if any(historico_fila.get(t, {}).get(nk) for t in range(tick_maximo + 1))
        ]

        linhas_fila = ''
        for idx, (nivel_key, nivel_label) in enumerate(niveis_ativos):
            # Borda separadora apenas na primeira linha da fila
            sep = 'border-top:2px solid #475569;' if idx == 0 else ''
            label_s = _fixo_fila + sep
            celulas = ''
            for t in range(tick_maximo + 1):
                pids = historico_fila.get(t, {}).get(nivel_key, [])
                if pids:
                    items = ''.join(
                        f'<div style="color:{_cor_por_pid(p)[0]};font-size:11px;'
                        f'font-weight:bold;line-height:16px;text-align:center;">#{p}</div>'
                        for p in pids
                    )
                    celulas += (
                        f'<td style="border:1px solid #334155;{sep}'
                        f'background:#0f172a;min-width:30px;padding:2px 4px;vertical-align:top;">'
                        f'{items}</td>'
                    )
                else:
                    celulas += (
                        f'<td style="border:1px solid #1e293b;{sep}'
                        f'background:#0f172a;min-width:30px;height:20px;"></td>'
                    )
            linhas_fila += f'<tr><th style="{label_s}">{nivel_label}</th>{celulas}</tr>'

        if linhas_fila:
            tfoot_html = f'<tfoot>{linhas_fila}</tfoot>'

    if not recursos_ativos:
        corpo = (
            f'<tr><td colspan="{tick_maximo + 2}" '
            'style="color:#64748b;padding:12px;text-align:center;">'
            'Nenhuma atividade de recurso registrada.</td></tr>'
        )
    else:
        corpo = linhas_html

    html = (
        '<div style="overflow-x:auto;width:100%;border:1px solid #334155;border-radius:4px;">'
        '<table style="border-collapse:collapse;white-space:nowrap;width:max-content;">'
        f'<thead><tr><th style="{_fixo}"></th>{cabecalhos_tick}</tr></thead>'
        f'<tbody>{corpo}</tbody>'
        f'{tfoot_html}'
        '</table></div>'
    )

    with container:
        ui.html(html).classes('w-full mt-2')
