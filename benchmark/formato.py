"""Parâmetros do ensaio e leitura do CSV real do LIKWID (perfctr -O -m)."""

import csv
import math
import re

TAMANHOS = (32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 9000, 10000, 20000)
REGIOES = ("Newton_Total", "Jacobiana", "Sistema_Linear")
VERSOES = ("baseline", "otimizada")
PASSOS = 25
GRUPOS = (("MEM", "L3"), ("L2CACHE", "L1CACHE", "CACHE"), ("FLOPS_DP",), ("FLOPS_AVX",))
CAMPOS = ("n", "versao", "repeticao", "regiao", "grupo", "metrica", "valor",
          "chamadas", "tempo_marcador_s", "timestamp_ms", "parede_ms")

METRICAS = {
    "MEM": ("memory bandwidth [mbytes/s]",),
    "L3": ("l3 bandwidth [mbytes/s]", "l3 data bandwidth [mbytes/s]"),
    "L2CACHE": ("l2 data cache miss ratio", "data cache miss ratio", "l2 cache miss ratio", "l2 miss ratio"),
    "L1CACHE": ("l1 data cache miss ratio", "data cache miss ratio", "l1 miss ratio", "l1d miss ratio"),
    "CACHE": ("data cache miss ratio", "l1 data cache miss ratio", "l1d miss ratio"),
    "FLOPS_DP": ("dp [mflop/s]", "double precision [mflop/s]", "mflop/s"),
    "FLOPS_AVX": ("packed dp [mflop/s]", "avx dp [mflop/s]", "dp [mflop/s]"),
}


def normaliza(nome):
    return " ".join(nome.casefold().split())


def numero(campo):
    """Aceita zero medido e rejeita métricas indefinidas ou fora do domínio."""
    try:
        valor = float(campo)
    except ValueError as erro:
        raise ValueError(f"valor de LIKWID indefinido ({campo!r})") from erro
    if not math.isfinite(valor) or valor < 0:
        raise ValueError(f"valor de LIKWID inválido ({campo!r})")
    return valor


def le_likwid(caminho, grupo, cpu, metrica=None):
    """Lê apenas as colunas da CPU medida em Raw e Metric, nunca em STAT."""
    informacoes = {r: {} for r in REGIOES}
    metricas = {r: {} for r in REGIOES}
    regiao, tabela, coluna = None, "", None
    with caminho.open(encoding="utf-8-sig", newline="") as entrada:
        for linha in csv.reader(entrada):
            linha = [campo.strip() for campo in linha]
            if not linha or not linha[0]:
                continue
            if linha[0] == "TABLE":
                regiao = None
                coluna = None
                if len(linha) >= 4 and linha[1].startswith("Region ") and linha[3] == grupo:
                    nome = linha[1][len("Region "):]
                    if nome in REGIOES:
                        regiao = nome
                        tabela = linha[2].split()[-1]
                continue
            if regiao is None:
                continue
            cabecalho = "Region Info" if tabela == "Raw" else "Metric" if tabela == "Metric" else ""
            if linha[0] == cabecalho:
                hwthread = f"HWThread {cpu}"
                coluna = linha.index(hwthread) if hwthread in linha else None
            elif linha[0] == "Event":
                coluna = None
            elif coluna is not None and coluna < len(linha):
                destino = informacoes if tabela == "Raw" else metricas
                destino[regiao][normaliza(linha[0])] = (linha[0], linha[coluna])

    dados = {}
    candidatos = (normaliza(metrica),) if metrica else METRICAS[grupo]
    for regiao in REGIOES:
        info = informacoes[regiao]
        if "call count" not in info:
            raise ValueError(f"{grupo}/{regiao}: marcador ou HWThread {cpu} ausente")
        chamadas = numero(info["call count"][1])
        if chamadas != PASSOS:
            raise ValueError(f"{grupo}/{regiao}: {chamadas:g} chamadas, esperadas {PASSOS}")
        campo_tempo = info.get("rdtsc runtime [s]", info.get("runtime (rdtsc) [s]"))
        if campo_tempo is None:
            raise ValueError(f"{grupo}/{regiao}: tempo do marcador ausente")
        disponiveis = metricas[regiao]
        candidato = next((nome for nome in candidatos if nome in disponiveis), None)
        if candidato is None:
            nomes = ", ".join(nome for nome, _ in disponiveis.values())
            raise ValueError(f"{grupo}/{regiao}: métrica esperada ausente. Disponíveis: {nomes}")
        nome, valor = disponiveis[candidato]
        dados[regiao] = (nome, numero(valor), int(chamadas), numero(campo_tempo[1]))
    if len({dado[0] for dado in dados.values()}) != 1:
        raise ValueError(f"{grupo}: rótulos incompatíveis entre regiões")
    return dados


def le_tempos(stderr):
    """Extrai as 25 atualizações e os três tempos do canal de coleta."""
    linhas = re.findall(r"^BROYDEN_STATS,(.*)$", stderr, re.MULTILINE)
    if len(linhas) != 1:
        raise ValueError("linha BROYDEN_STATS ausente ou duplicada")
    campos = linhas[0].split(",")
    if len(campos) != 4 or campos[0] != str(PASSOS):
        raise ValueError(f"execução sem as {PASSOS} atualizações exigidas: {linhas[0]}")
    return dict(zip(REGIOES, map(numero, campos[1:])))
