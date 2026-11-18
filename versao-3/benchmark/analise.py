#!/usr/bin/env python3
"""Valida as amostras da campanha, resume os dados e gera gráficos em PDF."""

import argparse
import csv
import json
from pathlib import Path
import statistics
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import NullFormatter, ScalarFormatter

from formato import GRUPOS, PASSOS, REGIOES, TAMANHOS, VERSOES, numero

REGIAO = dict(zip(REGIOES, ("Método de Newton", "Construção da Jacobiana", "Solução do sistema linear")))
REGIAO_PROSA = dict(zip(REGIOES, ("o método de Newton", "a construção da Jacobiana", "a solução do sistema linear")))
TITULOS = {"tempo": "Tempo de execução", "banda": "Banda de memória", "cache": "Taxa de faltas em cache",
           "flops": "Operações de ponto flutuante"}
FAMILIAS = {"tempo": ("tempo",), "banda": GRUPOS[0], "cache": GRUPOS[1],
            "flops": ("FLOPS_DP", "FLOPS_AVX")}
EIXOS = {"tempo": "Tempo acumulado [ms]", "banda": "Banda [MB/s]", "cache": "Taxa de faltas [fração]",
         "flops": "Taxa [MFLOP/s]"}
CORES = {"densa": "#B74624", "baseline": "#25548B", "otimizada": "#13754E",
         "baseline-original": "#5981B0", "otimizada-original": "#5D9A82"}
MARCAS = {"densa": "s", "otimizada": "^"}
ROTULOS = {"densa": "Densa", "baseline": "Em banda", "otimizada": "Otimizada",
           "baseline-original": "Básica original", "otimizada-original": "Otimizada original"}


def agrega(pasta, meta):
    """Exige todas as amostras nas dimensões previstas para cada versão."""
    grupos = meta["grupos_concluidos"]
    versoes = meta.get("versoes", VERSOES)
    chaves = {(grupo, regiao, versao, n): {} for grupo in grupos for regiao in REGIOES
              for versao in versoes for n in TAMANHOS
              if versao != "densa" or n <= meta["max_n_densa"]}
    esperadas = set(range(1, meta["repeticoes"] + 1))
    with (pasta / "medicoes.csv").open(encoding="utf-8", newline="") as arquivo:
        for linha in csv.DictReader(arquivo):
            grupo = linha["grupo"]
            if grupo not in grupos:
                continue
            chave = (grupo, linha["regiao"], linha["versao"], int(linha["n"]))
            metrica = meta["metricas_likwid"].get(grupo, "timestamp [ms]")
            repeticao = int(linha["repeticao"])
            if (chave not in chaves or repeticao in chaves[chave] or int(linha["chamadas"]) != PASSOS
                    or linha["metrica"] != metrica):
                raise ValueError(f"amostra inválida ou duplicada: {chave}, repetição {repeticao}")
            chaves[chave][repeticao] = numero(linha["valor"])

    resumo = {}
    for chave, amostras in chaves.items():
        if amostras.keys() != esperadas:
            raise ValueError(f"grade incompleta ou repetição inesperada em {chave}")
        valores = list(amostras.values())
        resumo[chave] = {"media": statistics.fmean(valores), "mediana": statistics.median(valores),
                         "desvio": statistics.stdev(valores) if len(valores) > 1 else 0.0,
                         "minimo": min(valores), "maximo": max(valores), "repeticoes": len(valores)}
    return resumo


def escreve_resumo(pasta, resumo):
    with (pasta / "resumo.csv").open("w", encoding="utf-8", newline="") as arquivo:
        escritor = csv.writer(arquivo)
        campos = ("media", "mediana", "desvio", "minimo", "maximo", "repeticoes")
        escritor.writerow(("grupo", "regiao", "versao", "n", *campos))
        for chave, valores in resumo.items():
            escritor.writerow((*chave, *(valores[campo] for campo in campos)))


def escreve_comparacao(pasta, resumo):
    """Calcula speedup pela razão entre as médias dos tempos."""
    pares = [("baseline", "otimizada")]
    if ("tempo", REGIOES[0], "densa", TAMANHOS[0]) in resumo:
        pares.extend([("densa", "baseline"), ("densa", "otimizada")])
    if ("tempo", REGIOES[0], "baseline-original", TAMANHOS[0]) in resumo:
        pares.extend([("baseline-original", "otimizada-original"),
                      ("baseline-original", "baseline"), ("otimizada-original", "otimizada")])
    with (pasta / "comparacao.csv").open("w", encoding="utf-8", newline="") as arquivo:
        escritor = csv.writer(arquivo)
        escritor.writerow(("n", "regiao", "referencia", "comparada", "media_referencia_ms",
                           "media_comparada_ms", "speedup"))
        for antes, depois in pares:
            for regiao in REGIOES:
                for n in TAMANHOS:
                    a = resumo.get(("tempo", regiao, antes, n))
                    b = resumo.get(("tempo", regiao, depois, n))
                    if a and b:
                        escritor.writerow((n, regiao, antes, depois, a["media"], b["media"],
                                           a["media"] / b["media"]))


def gera_figura(pasta, familia, regiao, grupos, dados, meta):
    """Gera uma figura vetorial com a mesma cor para cada versão entre painéis."""
    fig, ax = plt.subplots(figsize=(5.4, 3.0), layout="constrained")
    for grupo in grupos:
        avx = grupo == "FLOPS_AVX"
        versoes = meta.get("versoes", VERSOES)
        for versao in versoes:
            tamanhos = [n for n in TAMANHOS if (grupo, regiao, versao, n) in dados]
            valores = [dados[grupo, regiao, versao, n] for n in tamanhos]
            medias = [v["media"] for v in valores]
            if grupo == "tempo" and any(media <= 0 for media in medias):
                raise ValueError(f"tempo médio não positivo em {regiao}/{versao}")
            cor = CORES.get(versao)
            rotulo = ROTULOS.get(versao, versao.capitalize())
            nome = f"{rotulo} · {grupo}" if familia == "flops" else rotulo
            estilo = ":" if versao.endswith("-original") else "--" if avx else "-"
            linha, = ax.plot(tamanhos, medias, color=cor, linestyle=estilo,
                             marker="s" if avx else MARCAS.get(versao, "o"), markersize=3.3,
                             linewidth=1.5, label=nome)
            if familia == "tempo":
                inferior = [v["media"] - v["desvio"] if v["media"] > v["desvio"] else float("nan")
                            for v in valores]
                superior = [v["media"] + v["desvio"] for v in valores]
                ax.fill_between(tamanhos, inferior, superior, color=linha.get_color(),
                                alpha=0.13, linewidth=0)

    cap = f"{TITULOS[familia]} · {REGIAO[regiao]}"
    sub = ", ".join(f"{g} · {meta['metricas_likwid'][g]}" for g in grupos if g != "tempo")
    ax.set_title(cap + (f"\n{sub}" if sub else ""), loc="left", fontsize=10.5, pad=7)
    ax.set_xscale("log")
    ax.set_xlabel("Dimensão do sistema (N)")
    ax.set_ylabel(EIXOS[familia])
    x_major = (32, 128, 512, 2000, 8000, 20000)
    ax.set_xticks(x_major, [f"{n:,}".replace(",", ".") for n in x_major])
    ax.xaxis.set_minor_formatter(NullFormatter())
    if familia == "tempo":
        ax.set_yscale("log")
    else:
        ax.set_ylim(bottom=0)
        ax.yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
    ax.tick_params(labelsize=8)
    ax.grid(axis="y", color="#D8DDE4", linewidth=0.55)
    ax.spines[["top", "right"]].set_visible(False)
    ax.spines[["left", "bottom"]].set_color("#9AA5B1")
    ax.legend(loc="best", fontsize=8, frameon=True, framealpha=0.94, edgecolor="#E3E7EB")
    nome = f"{familia}-{regiao.lower()}.pdf"
    fig.savefig(pasta / nome, metadata={"Title": cap})
    plt.close(fig)
    return nome


def gera_speedup(pasta, dados):
    """Mostra o ganho da revisão em relação à mesma versão original."""
    fig, paineis = plt.subplots(1, 2, figsize=(9, 3.5), sharey=True, layout="constrained")
    for ax, versao in zip(paineis, VERSOES):
        for regiao in REGIOES:
            razoes = [dados["tempo", regiao, f"{versao}-original", n]["media"]
                      / dados["tempo", regiao, versao, n]["media"] for n in TAMANHOS]
            ax.plot(TAMANHOS, razoes, marker="o", markersize=3, label=REGIAO[regiao])
        ax.axhline(1, linestyle="--", color="#626B75", linewidth=1)
        ax.set_xscale("log")
        ax.set_xlabel("Dimensão do sistema (N)")
        ax.set_title(ROTULOS[versao])
        ax.grid(axis="y", alpha=0.25)
    paineis[0].set_ylabel("Speedup (original / revisada)")
    paineis[1].legend(fontsize=8)
    fig.savefig(pasta / "speedup-revisao.pdf")
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campanha", type=Path)
    args = parser.parse_args()
    pasta = args.campanha.resolve()
    meta = json.loads((pasta / "campanha.json").read_text(encoding="utf-8"))
    dados = agrega(pasta, meta)
    if "tempo" not in meta["grupos_concluidos"]:
        raise ValueError("uma campanha de tempo completa é necessária")
    escreve_resumo(pasta, dados)
    escreve_comparacao(pasta, dados)
    graficos = pasta / "graficos"
    graficos.mkdir(exist_ok=True)
    total = 0
    if ("tempo", REGIOES[0], "otimizada-original", TAMANHOS[0]) in dados:
        gera_speedup(graficos, dados)
    for familia, opcoes in FAMILIAS.items():
        grupos = [g for g in opcoes if g in meta["grupos_concluidos"]]
        if grupos:
            for regiao in REGIOES:
                gera_figura(graficos, familia, regiao, grupos, dados, meta)
                total += 1
    extra = int(("tempo", REGIOES[0], "otimizada-original", TAMANHOS[0]) in dados)
    print(f"{total + extra} gráficos gerados em {graficos}.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError) as erro:
        print(f"Análise: {erro}", file=sys.stderr)
        sys.exit(1)
