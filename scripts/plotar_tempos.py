#!/usr/bin/env python3
"""Gera os três gráficos de tempo com médias de um ou mais CSVs de medição."""
import argparse
import collections
import csv
import pathlib
import statistics
import matplotlib.pyplot as plt

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('csv', nargs='+', type=pathlib.Path)
p.add_argument('--saida', type=pathlib.Path, default=pathlib.Path('resultados/graficos'))
a = p.parse_args()
a.saida.mkdir(parents=True, exist_ok=True)
rows = []
for path in a.csv:
    with path.open() as f: rows.extend(csv.DictReader(f))
for field, title in [('newton_ms', 'Método de Newton'),
                     ('jacobiana_ms', 'Construção das Jacobianas'),
                     ('linear_ms', 'Resolução dos sistemas lineares')]:
    series = collections.defaultdict(lambda: collections.defaultdict(list))
    for row in rows:
        series[row['versao']][int(row['N'])].append(float(row[field]))
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for version, values in sorted(series.items()):
        sizes = sorted(values)
        means = [statistics.mean(values[n]) for n in sizes]
        if any(v <= 0 for v in means):
            raise ValueError('Tempo não positivo: resolução do relógio insuficiente')
        ax.plot(sizes, means, marker='o', label=version)
    ax.set(xscale='log', yscale='log', xlabel='Dimensão N',
           ylabel='Tempo médio (ms)', title=title)
    ax.grid(True, which='both', alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(a.saida / f'{field}.pdf')
    plt.close(fig)
