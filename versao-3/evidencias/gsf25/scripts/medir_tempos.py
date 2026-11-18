#!/usr/bin/env python3
"""Coleta tempos internos, sem incluir printf, em execuções independentes."""
import argparse
import csv
import pathlib
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--exe', default='./broyden')
p.add_argument('--versao', default='otimizada')
p.add_argument('--repeticoes', type=int, default=10)
p.add_argument('--sem-aquecimento', action='store_true',
               help='Omite a execução de aquecimento em campanhas adicionais')
p.add_argument('--x0', type=float, default=-1.0)
p.add_argument('--saida', type=pathlib.Path, default=pathlib.Path('resultados/tempos.csv'))
p.add_argument('--cpu', type=int, help='Fixa afinidade usando taskset (Linux)')
p.add_argument('--tamanhos', nargs='+', type=int,
               default=[32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 9000, 10000, 20000])
a = p.parse_args()
if any(n < 2 for n in a.tamanhos): p.error('tamanhos devem ser >=2')
if a.repeticoes < 1: p.error('repeticoes deve ser >=1')
exe = str(pathlib.Path(a.exe).resolve())
command = (["taskset", "-c", str(a.cpu)] if a.cpu is not None else []) + [exe]
a.saida.parent.mkdir(parents=True, exist_ok=True)
with a.saida.open('w', newline='') as out:
    w = csv.writer(out)
    w.writerow(['versao', 'N', 'repeticao', 'newton_ms', 'jacobiana_ms', 'linear_ms'])
    for n in a.tamanhos:
        data = f'{n} {a.x0} 0 25\n'.encode()
        # Uma execução de aquecimento por tamanho, excluída da média.
        if not a.sem_aquecimento:
            subprocess.run(command, input=data, stdout=subprocess.DEVNULL, check=True)
        for k in range(a.repeticoes):
            # Arquivo temporário evita reter o histórico textual em memória Python.
            import tempfile
            with tempfile.TemporaryFile() as raw:
                subprocess.run(command, input=data, stdout=raw, check=True)
                raw.seek(max(0, raw.tell()-1024))
                tail = raw.read().decode()
                times = tail.split('###########\n')[-2].splitlines()
            if len(times) != 3: raise RuntimeError('Formato de saída inválido')
            w.writerow([a.versao, n, k+1, *map(float, times)])
        out.flush()
        print(f'N={n}: {a.repeticoes} repetições')
