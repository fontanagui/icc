#!/usr/bin/env python3
"""Compara todos os estados das duas versões; tempos naturalmente diferem."""
import math
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
executables = [root / 'broyden', root / 'baseline' / 'broyden']

def states(executable, data):
    result = subprocess.run([str(executable)], input=data, text=True,
                            capture_output=True, check=True)
    body = result.stdout.split('###########\n')[0]
    return [[float(line.split(' = ')[1]) for line in block.splitlines()]
            for block in body.strip().split('\n#\n')]

cases = [(n, -1, 0, 25) for n in (2, 3, 8, 32, 64, 128, 256, 512)]
cases += [(n, 0.75, 0, 1) for n in (2, 4, 8)]
cases += [(32, -1, 1e-10, 25), (32, -1, 0, 0), (8, 0.6, 0, 4)]
for n, x0, epsilon, maxit in cases:
    data = f'{n} {x0} {epsilon} {maxit}\n'
    a, b = [states(exe, data) for exe in executables]
    assert len(a) == len(b), (data, 'Quantidade de estados diferente')
    for step, (x, y) in enumerate(zip(a, b)):
        assert len(x) == len(y) == n
        assert all(math.isclose(u, v, abs_tol=1e-10, rel_tol=1e-10)
                   for u, v in zip(x, y)), (data, step)
    print(f'OK comparação N={n}, x0={x0}, epsilon={epsilon}, MAX={maxit}')
