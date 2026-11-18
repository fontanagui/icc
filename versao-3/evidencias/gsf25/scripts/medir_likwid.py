#!/usr/bin/env python3
"""Coleta métricas LIKWID por região, preservando os CSVs originais."""

import argparse
import csv
import math
import os
from pathlib import Path
import subprocess
import tempfile


TAMANHOS = [32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 9000, 10000, 20000]
CHAMADAS = {'NEWTON': 1, 'JACOBIANA': 25, 'SISTEMA_LINEAR': 25}
METRICAS = {
    'MEM': {'banda_memoria': ('Memory bandwidth [MBytes/s]',)},
    'L3': {'banda_l3': ('L3 bandwidth [MBytes/s]',)},
    'L2CACHE': {'miss_l2': ('L2 miss ratio', 'L2 data cache miss ratio',
                            'data cache miss ratio')},
    'L1CACHE': {'miss_l1': ('L1D miss ratio', 'L1 data cache miss ratio',
                            'data cache miss ratio')},
    'CACHE': {'miss_l1': ('L1D miss ratio', 'L1 data cache miss ratio',
                          'data cache miss ratio')},
    'FLOPS_DP': {'flops_dp': ('DP [MFLOP/s]', 'MFLOP/s'),
                 'flops_dp_avx': ('AVX DP [MFLOP/s]',)},
    'FLOPS_AVX': {'flops_avx': ('Packed DP [MFLOP/s]', 'AVX DP [MFLOP/s]')},
}


def ler_metricas(arquivo, grupo, cpu):
    regiao = tabela = None
    chamadas = {}
    valores = {}
    with arquivo.open(newline='') as entrada:
        for linha in csv.reader(entrada):
            if not linha:
                continue
            if linha[0] == 'TABLE':
                regiao = linha[1].removeprefix('Region ')
                if linha[3] != grupo:
                    raise ValueError(f'grupo inesperado: {linha[3]}')
                tabela = linha[2].split()[-1]
            elif regiao in CHAMADAS and linha[0] in ('Region Info', 'Metric'):
                if linha[1] != f'HWThread {cpu}':
                    raise ValueError(f'núcleo inesperado: {linha[1]}')
            elif regiao in CHAMADAS and tabela == 'Raw' and linha[0] == 'call count':
                chamadas[regiao] = int(linha[1])
            elif regiao in CHAMADAS and tabela == 'Metric':
                for metrica, nomes in METRICAS[grupo].items():
                    if linha[0].casefold() in {nome.casefold() for nome in nomes}:
                        valor = float(linha[1])
                        if not math.isfinite(valor) or valor < 0:
                            raise ValueError(f'{regiao}: {linha[0]} inválido ({valor})')
                        valores[regiao, metrica] = valor
    esperadas = {(regiao, metrica) for regiao in CHAMADAS for metrica in METRICAS[grupo]}
    if chamadas != CHAMADAS or valores.keys() != esperadas:
        raise ValueError(f'marcadores ou métricas incompletos: {chamadas}, {valores}')
    return valores


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpu', required=True, type=int)
    parser.add_argument('--perfctr', default='likwid-perfctr')
    parser.add_argument('--baseline', type=Path, default=Path('baseline/broyden'))
    parser.add_argument('--otimizada', type=Path, default=Path('otimizada/broyden'))
    parser.add_argument('--versoes', nargs='+', choices=('baseline', 'otimizada'),
                        default=['baseline', 'otimizada'])
    parser.add_argument('--grupos', nargs='+', choices=METRICAS,
                        default=['MEM', 'L2CACHE', 'FLOPS_DP', 'FLOPS_AVX'])
    parser.add_argument('--tamanhos', nargs='+', type=int, default=TAMANHOS)
    parser.add_argument('--repeticoes', type=int, default=1)
    parser.add_argument('--saida', type=Path, required=True)
    args = parser.parse_args()
    if args.repeticoes < 1 or min(args.tamanhos) < 2:
        parser.error('use repetições >= 1 e tamanhos >= 2')

    args.saida.parent.mkdir(parents=True, exist_ok=True)
    originais = args.saida.parent / args.saida.stem
    originais.mkdir(exist_ok=True)
    os.environ['LC_ALL'] = 'C'
    with args.saida.open('w', newline='') as saida:
        gravar = csv.writer(saida)
        gravar.writerow(['versao', 'N', 'x0', 'epsilon', 'MAX', 'cpu', 'repeticao',
                          'grupo', 'regiao', 'metrica', 'valor', 'arquivo_likwid'])
        saida.flush()
        for n in args.tamanhos:
            for grupo in args.grupos:
                for repeticao in range(1, args.repeticoes + 1):
                    for versao in args.versoes:
                        exe = getattr(args, versao).resolve()
                        bruto = originais / f'{versao}_{n}_{grupo}_{repeticao}.csv'
                        incompleto = bruto.with_suffix('.incompleto.csv')
                        if bruto.exists():
                            valores = ler_metricas(bruto, grupo, args.cpu)
                        else:
                            comando = [args.perfctr, '-O', '-C', str(args.cpu), '-g', grupo,
                                       '-m', '-o', str(incompleto.resolve()), str(exe)]
                            with tempfile.TemporaryFile() as log:
                                processo = subprocess.run(comando, input=f'{n} -1 0 25\n'.encode(),
                                                          stdout=log, stderr=log)
                                try:
                                    processo.check_returncode()
                                    valores = ler_metricas(incompleto, grupo, args.cpu)
                                except (OSError, ValueError, subprocess.CalledProcessError) as erro:
                                    log.seek(max(0, log.tell() - 8192))
                                    bruto.with_suffix('.erro.log').write_bytes(log.read())
                                    raise SystemExit(f'{versao}, N={n}, {grupo}: {erro}') from erro
                            incompleto.replace(bruto)
                        for (regiao, metrica), valor in valores.items():
                            gravar.writerow([versao, n, -1, 0, 25, args.cpu, repeticao, grupo,
                                             regiao, metrica, valor,
                                             bruto.relative_to(args.saida.parent)])
                        saida.flush()
                        print(f'{versao}, N={n}, {grupo}, repetição {repeticao}', flush=True)


if __name__ == '__main__':
    main()
