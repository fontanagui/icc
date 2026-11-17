#!/usr/bin/env python3
"""Validação independente por Gauss denso, formato, bordas e tamanhos exigidos."""
import argparse
import math
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('exe', nargs='?', default='./broyden')
parser.add_argument('--rapido', action='store_true', help='Limita os testes grandes a N=256')
args = parser.parse_args()
EXE = args.exe
SIZES = (32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 9000, 10000, 20000)


def function(x):
    n = len(x)
    return [(3 - 2*x[i])*x[i] - (x[i-1] if i else 0)
            - (2*x[i+1] if i+1 < n else 0)
            + (1 if i+1 < n else 0) for i in range(n)]


def dense_step(x):
    n = len(x)
    a = [[0.0]*n for _ in range(n)]
    b = [-v for v in function(x)]
    for i in range(n):
        a[i][i] = 3 - 4*x[i]
        if i: a[i][i-1] = -1
        if i+1 < n: a[i][i+1] = -2
    for j in range(n):
        p = max(range(j, n), key=lambda i: abs(a[i][j]))
        a[p], a[j] = a[j], a[p]
        b[p], b[j] = b[j], b[p]
        assert abs(a[j][j]) > 1e-14, 'Referência singular'
        for i in range(j+1, n):
            factor = a[i][j]/a[j][j]
            for k in range(j, n):
                a[i][k] -= factor*a[j][k]
            b[i] -= factor*b[j]
    s = [0.0]*n
    for i in range(n-1, -1, -1):
        s[i] = (b[i] - sum(a[i][j]*s[j] for j in range(i+1, n)))/a[i][i]
    return [u+v for u, v in zip(x, s)]


def run(n, x0=-1, epsilon=0, maxit=25):
    p = subprocess.run([EXE], input=f'{n} {x0} {epsilon} {maxit}\n',
                       text=True, capture_output=True, check=True)
    parts = p.stdout.split('###########\n')
    assert len(parts) == 3 and parts[-1] == ''
    times = [float(v) for v in parts[1].splitlines()]
    assert len(times) == 3 and all(math.isfinite(t) and t >= 0 for t in times)
    assert times[0]+1e-6 >= times[1]+times[2]
    history = []
    for block in parts[0].strip().split('\n#\n'):
        lines = block.splitlines()
        assert len(lines) == n
        x = []
        for i, line in enumerate(lines):
            name, value = line.split(' = ')
            assert name == f'x{i+1}'
            x.append(float(value))
        assert all(math.isfinite(v) for v in x)
        history.append(x)
    assert history[0] == [x0]*n
    return history


# A referência armazena matriz cheia e usa Gauss genérico (não o solver otimizado).
for n in (2, 3, 5, 8, 32):
    for x0 in (-1, 0, 0.6):
        history = run(n, x0, maxit=4)
        ref = [x0]*n
        for actual in history[1:]:
            ref = dense_step(ref)
            assert all(math.isclose(a, b, rel_tol=1e-8, abs_tol=1e-8)
                       for a, b in zip(actual, ref)), (n, x0)
# Diagonal inicial zero: exige troca de linhas; Thomas sem pivoteamento falharia.
for n in (2, 4, 8):
    actual = run(n, 0.75, maxit=1)[-1]
    ref = dense_step([0.75]*n)
    assert all(math.isclose(a, b, abs_tol=1e-12) for a, b in zip(actual, ref))
for n in (SIZES[:4] if args.rapido else SIZES):
    history = run(n)
    assert len(history) == 26, (n, len(history))
    assert max(map(abs, function(history[-1]))) < 1e-11, n
    print(f'OK N={n}: 25 atualizações, resíduo < 1e-11')
assert len(run(2, maxit=0)) == 1
assert len(run(32, epsilon=1e-10)) < 26
for invalid in ('', '1 -1 0 25', '0 -1 0 25', '2 nan 0 25', '2 -1 -1 25',
                '2 -1 0 -1', 'abc', '2 -1 inf 25'):
    p = subprocess.run([EXE], input=invalid, text=True, capture_output=True)
    assert p.returncode != 0 and not p.stdout
# N ímpar e diagonal zero: matriz singular deve gerar erro controlado.
p = subprocess.run([EXE], input='3 0.75 0 1\n', text=True, capture_output=True)
assert p.returncode != 0 and not p.stdout
print('OK: referência densa, pivoteamento, tolerância, MAX=0, entradas inválidas e singularidade')
