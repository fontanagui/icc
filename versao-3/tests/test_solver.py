#!/usr/bin/env python3
"""Confere a interface, as condições de parada e o passo de Newton."""

import argparse
import math
import os
from pathlib import Path
import resource
import struct
import subprocess


def residuo(x, ultima):
    """Avaliação independente do polinômio, com zeros explícitos nas bordas."""
    estendido = [0.0, *x, 0.0]
    return [3 * xi - 2 * xi**2 - estendido[i] - 2 * estendido[i + 2]
            + (ultima if i == len(x) - 1 else 1)
            for i, xi in enumerate(x)]


def gauss_denso(a, b):
    """Eliminação densa para referência independente nas dimensões pequenas."""
    n = len(b)
    for k in range(n):
        p = max(range(k, n), key=lambda i: abs(a[i][k]))
        assert a[p][k] != 0.0
        a[k], a[p] = a[p], a[k]
        b[k], b[p] = b[p], b[k]
        for i in range(k + 1, n):
            m = a[i][k] / a[k][k]
            for j in range(k + 1, n):
                a[i][j] -= m * a[k][j]
            b[i] -= m * b[k]
    for i in reversed(range(n)):
        b[i] = (b[i] - sum(a[i][j] * b[j] for j in range(i + 1, n))) / a[i][i]
    return b


def confere_passos(trajetoria, ultima):
    """Confere J delta + F e compara os vetores com o passo denso."""
    for x, novo in zip(trajetoria, trajetoria[1:]):
        f = residuo(x, ultima)
        n = len(x)
        a = [[0.0] * n for _ in x]
        for i, xi in enumerate(x):
            a[i][i] = 3.0 - 4.0 * xi
            if i:
                a[i][i - 1] = -1.0
            if i + 1 < n:
                a[i][i + 1] = -2.0
        delta = [b - a for a, b in zip(x, novo)]
        for linha, fi in zip(a, f):
            termos = [c * v for c, v in zip(linha, delta)]
            assert abs(sum(termos) + fi) <= 2e-11 * (1 + abs(fi) + sum(map(abs, termos)))
        referencia = gauss_denso(a, [-fi for fi in f])
        for xi, passo, atualizado in zip(x, referencia, novo):
            assert math.isclose(xi + passo, atualizado, abs_tol=2e-12, rel_tol=2e-11)


def le_saida(saida, n):
    """Verifica a sintaxe inteira, os índices, os separadores e os tempos."""
    linhas = saida.splitlines()
    assert linhas[-5] == linhas[-1] == "###########"
    tempos = list(map(float, linhas[-4:-1]))
    assert all(math.isfinite(v) and v >= 0 for v in tempos)
    assert tempos[0] + 1e-8 >= sum(tempos[1:])

    assert len(linhas) >= n + 5
    assert (len(linhas) - 4) % (n + 1) == 0
    vetores = []
    for inicio in range(0, len(linhas) - 5, n + 1):
        if inicio > 0:
            assert linhas[inicio - 1] == "#"
        vetor = []
        for i in range(n):
            chave, valor = linhas[inicio + i].split(" = ")
            assert chave == f"x{i + 1}"
            numero = float(valor)
            assert math.isfinite(numero)
            vetor.append(numero)
        vetores.append(vetor)
    return vetores, tempos


def executa(binario, entrada, **kwargs):
    """Roda uma instância independente e captura seu código de saída."""
    ambiente = os.environ.copy()
    ambiente.pop("BROYDEN_STATS", None)
    ambiente["LC_ALL"] = "C"
    return subprocess.run([binario], input=entrada, text=True, capture_output=True,
                          timeout=45, env=ambiente, **kwargs)


def limite_memoria():
    """Força malloc a falhar, sem pressionar a RAM do computador."""
    limite = 128 * 1024**2
    resource.setrlimit(resource.RLIMIT_AS, (limite, limite))


def valida(binario, ultima, sanitizers, limite_n=20000):
    """Exercita casos válidos, falhas e todas as dimensões da campanha."""
    casos = [(n, -1.0, 0, 25) for n in
             (1, 2, 3, 10, 32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 9000, 10000, 20000)]
    casos += [(1, -0.25, 1e-12, 25), (2, 0.75, 0, 2), (4, 0.75, 0, 1),
              (10, -0.25, 1e-12, 25), (3, -1.0, 0, 0), (1, 0, 2, 25)]
    casos = [caso for caso in casos if caso[0] <= limite_n]
    finais = {}
    max_residuo = 0.0

    for n, x0, epsilon, maximo in casos:
        resultado = executa(binario, f"{n}\n{x0}\n{epsilon}\n{maximo}\n")
        assert resultado.returncode == 0 and resultado.stderr == "", resultado.stderr
        vetores, tempos = le_saida(resultado.stdout, n)
        assert vetores[0] == [x0] * n
        if epsilon == 0:
            assert len(vetores) == maximo + 1
        if maximo == 0:
            assert len(vetores) == 1 and tempos == [0.0, 0.0, 0.0]
        if epsilon == 2:
            assert len(vetores) == 1 and tempos[1:] == [0.0, 0.0]
        erro = max(map(abs, residuo(vetores[-1], ultima)))
        if 0 < epsilon < 2:
            assert 1 < len(vetores) < maximo + 1 and erro <= epsilon
        if x0 == -1 and maximo == 25:
            assert erro < 2e-13, (n, erro)
            max_residuo = max(max_residuo, erro)
        if n <= 10:
            confere_passos(vetores, ultima)
        finais[(n, x0, epsilon, maximo)] = vetores[-1]

    entradas_invalidas = ("0 -1 0 25", "-3 -1 0 25", "3 NaN 0 25", "3 -1 -1 25",
                          "3 -1 0 -25", "3 -1 0", "3 -1 0 25lixo", "3 -1 inf 25",
                          "3.5 -1 0 25", "8\x00a -1 0 25", f"{'0' * 140}3 -1 0 25")
    for entrada in entradas_invalidas:
        falha = executa(binario, entrada)
        assert falha.returncode == 1 and "Entrada inválida" in falha.stderr
        assert falha.stdout == ""

    for n in (1, 3):
        singular = executa(binario, f"{n} .75 0 25")
        assert singular.returncode == 1 and "singular" in singular.stderr
        assert len(le_saida(singular.stdout, n)[0]) == 1
    for epsilon in (0, 1e-9):
        overflow = executa(binario, f"3 1e200 {epsilon} 1")
        assert overflow.returncode == 1 and "não finita" in overflow.stderr
        assert len(le_saida(overflow.stdout, 3)[0]) == 1

    if ultima == 1:
        limite = executa(binario, "1 0 1 25")
        assert limite.returncode == 0 and limite.stderr == ""
        assert le_saida(limite.stdout, 1)[0] == [[0.0]]
    else:
        raiz = executa(binario, "1 0 0 3")
        assert raiz.returncode == 0 and raiz.stderr == ""
        assert le_saida(raiz.stdout, 1)[0] == [[0.0]] * 4

    bits = 8 * struct.calcsize("P")
    grande = executa(binario, f"{(1 << bits) - 1} -1 0 1")
    assert grande.returncode == 1 and "memória insuficiente" in grande.stderr
    if not sanitizers:
        sem_memoria = executa(binario, "10000000 -1 0 1", preexec_fn=limite_memoria)
        assert sem_memoria.returncode == 1 and "memória insuficiente" in sem_memoria.stderr

    print(f"{Path(binario).name}: {len(casos)} casos válidos, max |F| no ensaio = {max_residuo:.3g}")
    return finais


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ultima", type=int, choices=(0, 1))
    parser.add_argument("otimizado", type=Path)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("--densa", type=Path)
    parser.add_argument("--sanitizers", action="store_true")
    args = parser.parse_args()
    otimizado = valida(args.otimizado.resolve(), args.ultima, args.sanitizers)
    baseline = valida(args.baseline.resolve(), args.ultima, args.sanitizers)
    for caso, vetor in otimizado.items():
        assert all(math.isclose(a, b, abs_tol=2e-12, rel_tol=2e-11)
                   for a, b in zip(vetor, baseline[caso])), caso
    if args.densa:
        densa = valida(args.densa.resolve(), args.ultima, args.sanitizers,
                       limite_n=128 if args.sanitizers else 512)
        for caso, vetor in densa.items():
            assert all(math.isclose(a, b, abs_tol=2e-12, rel_tol=2e-11)
                       for a, b in zip(vetor, otimizado[caso])), caso
    print("As versões concordam nos vetores finais dos casos em comum.")


if __name__ == "__main__":
    main()
