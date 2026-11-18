#!/usr/bin/env python3
"""Gera os gráficos de tempo e, se fornecidos, os gráficos LIKWID por região."""

import argparse
from collections import defaultdict
import csv
from pathlib import Path
from statistics import mean, stdev

import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator, ScalarFormatter


REGIOES = {
    'NEWTON': ('newton_ms', 'Método de Newton'),
    'JACOBIANA': ('jacobiana_ms', 'Construção das Jacobianas'),
    'SISTEMA_LINEAR': ('linear_ms', 'Resolução dos sistemas lineares'),
}
CORES = {'baseline': '#B54B23', 'baseline_densa': '#B54B23', 'otimizada': '#196391'}
ROTULOS = {'baseline': 'Baseline', 'baseline_densa': 'Baseline', 'otimizada': 'Otimizada'}
FAMILIAS_LIKWID = [
    ('mem', 'Banda de memória (MB/s)', [('banda_memoria', '', '-')]),
    ('l3', 'Banda L3 (MB/s)', [('banda_l3', '', '-')]),
    ('l2', 'Taxa de faltas L2', [('miss_l2', '', '-')]),
    ('l1', 'Taxa de faltas L1', [('miss_l1', '', '-')]),
    ('flops', 'Vazão (MFLOP/s)', [('flops_dp', 'DP', '-'), ('flops_avx', 'AVX DP', '--')]),
]


def ler_csv(caminhos):
    for caminho in caminhos:
        with caminho.open(newline='') as entrada:
            yield from csv.DictReader(entrada)


def checar_volumes(caminhos):
    """Em MEM, Newton contém as duas regiões internas e seu volume agregado."""
    checagens = {}
    for caminho in caminhos:
        for linha in ler_csv([caminho]):
            if linha['metrica'] != 'banda_memoria' or linha['regiao'] != 'NEWTON':
                continue
            bruto = caminho.parent / linha['arquivo_likwid']
            if not bruto.is_file():
                continue
            volumes = {}
            regiao = None
            with bruto.open(newline='') as entrada:
                for campos in csv.reader(entrada):
                    if campos and campos[0] == 'TABLE':
                        regiao = campos[1].removeprefix('Region ')
                    if campos and campos[0] == 'Memory data volume [GBytes]':
                        volumes[regiao] = float(campos[1])
            if volumes.keys() == REGIOES.keys():
                internos = volumes['JACOBIANA'] + volumes['SISTEMA_LINEAR']
                # Tolera arredondamento do volume exportado pelo LIKWID.
                chave = linha['versao'], int(linha['N'])
                inconsistente = volumes['NEWTON'] + 0.0002 < internos
                checagens[chave] = checagens.get(chave, False) or inconsistente
    return checagens


def plotar(saida, titulo, eixo_y, series, escala='linear', teto=None, volumes=None):
    fig, ax = plt.subplots(figsize=(6.0, 3.35), layout='constrained')
    tamanhos_totais = sorted({n for dados in series.values() for n in dados})
    marcou_volume = False
    for (versao, sufixo, estilo), dados in series.items():
        tamanhos = sorted(dados)
        medias = [mean(dados[n]) for n in tamanhos]
        if escala == 'log' and min(medias) <= 0:
            raise ValueError(f'O gráfico {saida.name} tem um tempo não positivo.')
        nome = ' '.join(filter(None, [ROTULOS.get(versao, versao), sufixo]))
        cor = CORES.get(versao)
        ax.plot(tamanhos, medias, color=cor, marker='s' if estilo == '--' else 'o',
                markersize=3.4, linewidth=1.3, linestyle=estilo, label=nome)
        if teto is not None:
            acima = [(n, v) for n, v in zip(tamanhos, medias) if v > teto]
            if acima:
                x, y = zip(*acima)
                ax.scatter(x, y, s=38, facecolors='none', edgecolors=cor, zorder=3)
        quebrados = [(n, v) for n, v in zip(tamanhos, medias)
                     if volumes and volumes.get((versao, n))]
        if quebrados:
            x, y = zip(*quebrados)
            ax.scatter(x, y, s=55, marker='x', color='#303539', zorder=4,
                       label=None if marcou_volume else 'Volume inconsistente')
            marcou_volume = True
    if teto is not None:
        ax.axhline(teto, color='#515861', linestyle=':', linewidth=1.1,
                   label='Teto nominal DDR4')
    ax.set(xscale='log', xlabel='Dimensão N', ylabel=eixo_y, title=titulo)
    if escala == 'symlog':
        ax.set_yscale(escala, linthresh=100)
        ax.set_ylim(bottom=0)
    else:
        ax.set_yscale(escala)
    ticks = tamanhos_totais[::2] if len(tamanhos_totais) > 7 else tamanhos_totais
    ticks = [n for i, n in enumerate(ticks) if i == 0 or n / ticks[i-1] >= 1.4]
    if ticks[-1] != tamanhos_totais[-1]:
        ticks.append(tamanhos_totais[-1])
    ax.xaxis.set_major_locator(FixedLocator(ticks))
    ax.xaxis.set_major_formatter(ScalarFormatter())
    ax.grid(True, which='major', color='#d9dee3', linewidth=0.6)
    ax.set_axisbelow(True)
    for lado in ('right', 'top'):
        ax.spines[lado].set_visible(False)
    ax.legend(fontsize=8, ncols=2 if len(series) > 2 else 1, framealpha=0.93)
    fig.savefig(saida)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', nargs='+', type=Path)
    parser.add_argument('--likwid', nargs='+', type=Path, default=[])
    parser.add_argument('--saida', type=Path, default=Path('resultados/graficos'))
    parser.add_argument('--resumo', type=Path)
    parser.add_argument('--teto-mem', type=float, help='Limite nominal de banda em MB/s')
    args = parser.parse_args()
    args.saida.mkdir(parents=True, exist_ok=True)

    tempos = list(ler_csv(args.csv))
    likwid = list(ler_csv(args.likwid))
    volumes = checar_volumes(args.likwid)
    amostras = defaultdict(list)
    for regiao, (campo, titulo) in REGIOES.items():
        series = defaultdict(lambda: defaultdict(list))
        for linha in tempos:
            versao, n, valor = linha['versao'], int(linha['N']), float(linha[campo])
            series[versao, '', '-'][n].append(valor)
            amostras[versao, n, regiao, 'tempo_ms'].append(valor)
        plotar(args.saida / f'{campo}.pdf', titulo, 'Tempo médio (ms)', series, escala='log')

        for familia, eixo_y, metricas in FAMILIAS_LIKWID:
            series = defaultdict(lambda: defaultdict(list))
            for metrica, sufixo, estilo in metricas:
                for linha in likwid:
                    if linha['regiao'] != regiao or linha['metrica'] != metrica:
                        continue
                    versao, n, valor = linha['versao'], int(linha['N']), float(linha['valor'])
                    series[versao, sufixo, estilo][n].append(valor)
                    amostras[versao, n, regiao, metrica].append(valor)
            if series:
                escala = 'linear'
                if familia == 'mem':
                    valores = (v for dados in series.values() for lista in dados.values()
                               for v in lista)
                    escala = 'log' if all(v > 0 for v in valores) else 'symlog'
                plotar(args.saida / f'{familia}_{regiao.lower()}.pdf', titulo, eixo_y, series,
                       escala=escala, teto=args.teto_mem if familia == 'mem' else None,
                       volumes=volumes if familia == 'mem' and regiao == 'NEWTON' else None)
    for linha in likwid:
        if linha['metrica'] == 'flops_dp_avx':
            chave = (linha['versao'], int(linha['N']), linha['regiao'], linha['metrica'])
            amostras[chave].append(float(linha['valor']))
    if args.resumo:
        args.resumo.parent.mkdir(parents=True, exist_ok=True)
        with args.resumo.open('w', newline='') as arquivo:
            gravar = csv.writer(arquivo)
            gravar.writerow(['versao', 'N', 'regiao', 'metrica', 'amostras', 'media',
                             'desvio_padrao_amostral', 'inconsistente_teto_mem',
                             'inconsistente_volume_mem'])
            for (versao, n, regiao, metrica), valores in sorted(amostras.items()):
                media = mean(valores)
                inconsistente = ''
                if metrica == 'banda_memoria' and args.teto_mem is not None:
                    inconsistente = int(media > args.teto_mem)
                volume = ''
                if metrica == 'banda_memoria' and regiao == 'NEWTON' and (versao, n) in volumes:
                    volume = int(volumes[versao, n])
                gravar.writerow([versao, n, regiao, metrica, len(valores), media,
                                 stdev(valores) if len(valores) > 1 else '', inconsistente,
                                 volume])


if __name__ == '__main__':
    main()
