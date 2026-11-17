#!/usr/bin/env python3
"""Resume amostras válidas e gera os gráficos e a seção de resultados em LaTeX."""

import argparse
import csv
import json
from pathlib import Path
import re
import shutil
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
CORES = dict(zip(VERSOES, ("#25548B", "#B74624")))
ROTULOS = dict(zip(VERSOES, ("Baseline", "Otimizada")))


def agrega(pasta, meta):
    """Exige a grade completa de tamanhos, versões, regiões e repetições."""
    grupos = meta["grupos_concluidos"]
    chaves = {(grupo, regiao, versao, n): {} for grupo in grupos for regiao in REGIOES
              for versao in VERSOES for n in TAMANHOS}
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


def gera_figura(pasta, familia, regiao, grupos, dados, meta):
    """Gera uma figura vetorial com a mesma cor para cada versão entre painéis."""
    fig, ax = plt.subplots(figsize=(6.8, 3.03), layout="constrained")
    for grupo in grupos:
        avx = grupo == "FLOPS_AVX"
        for versao in VERSOES:
            valores = [dados[grupo, regiao, versao, n] for n in TAMANHOS]
            medias = [v["media"] for v in valores]
            if grupo == "tempo" and any(media <= 0 for media in medias):
                raise ValueError(f"tempo médio não positivo em {regiao}/{versao}")
            cor = CORES[versao]
            nome = f"{ROTULOS[versao]} · {grupo}" if familia == "flops" else ROTULOS[versao]
            ax.plot(TAMANHOS, medias, color=cor, linestyle="--" if avx else "-",
                    marker="s" if avx else "o", markersize=3.3, linewidth=1.5, label=nome)
            if familia == "tempo":
                inferior = [v["media"] - v["desvio"] if v["media"] > v["desvio"] else float("nan")
                            for v in valores]
                superior = [v["media"] + v["desvio"] for v in valores]
                ax.fill_between(TAMANHOS, inferior, superior, color=cor, alpha=0.10, linewidth=0)

    cap = f"{TITULOS[familia]} · {REGIAO[regiao]}"
    sub = ", ".join(f"{g} · {meta['metricas_likwid'][g]}" for g in grupos if g != "tempo")
    ax.set_title(cap + (f"\n{sub}" if sub else ""), loc="left", fontsize=10.5, pad=7)
    ax.set_xscale("log")
    ax.set_xlabel("Dimensão do sistema (N)")
    ax.set_ylabel(EIXOS[familia])
    x_major = (32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 20000)
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


def tex(texto):
    """Protege dados de ambiente e métrica no LaTeX gerado."""
    substituicoes = {"\\": r"\textbackslash{}", "_": r"\_", "%": r"\%", "&": r"\&", "#": r"\#",
                     "{": r"\{", "}": r"\}", "$": r"\$", "~": r"\textasciitilde{}", "^": r"\textasciicircum{}"}
    return "".join(substituicoes.get(c, c) for c in str(texto))


def decimal(v, casas=3):
    """Escreve medidas pequenas com precisão visível em português."""
    if 0 < abs(v) < 10**(-casas):
        mantissa, exp = f"{v:.2e}".split("e")
        return f"${mantissa.replace('.', '{,}')}\\times 10^{{{int(exp)}}}$"
    return f"{v:.{casas}f}".replace(".", ",")


def descricao_ambiente(meta):
    cpu = tex(meta["cpu_modelo"] or "modelo indisponível")
    gcc = tex(meta["gcc"] or "versão indisponível")
    texto = (f"As execuções foram fixadas na CPU lógica {meta['cpu_logica']}. "
             rf"O sistema identificou o processador {cpu}. O compilador foi \emph{{{gcc}}}. ")
    tipo = re.search(r"(?mi)^CPU type:\s*([^\n\r]+)", meta.get("topologia_likwid", ""))
    if tipo and tipo[1].strip() != "nil":
        texto += f"O LIKWID identificou a microarquitetura como {tex(tipo[1].strip())}. "
    elif meta.get("alvo_gcc"):
        texto += rf"A detecção de arquitetura do GCC selecionou \texttt{{{tex(meta['alvo_gcc'])}}}. "
    base = meta.get("lscpu", {})
    topologia = [f"{tex(base[c])} {rotulo}" for c, rotulo in (("Socket(s)", "soquete(s)"),
                 ("Core(s) per socket", "núcleos por soquete"), ("Thread(s) per core", "threads por núcleo")) if c in base]
    if topologia:
        texto += "A topologia exposta inclui " + ", ".join(topologia) + ". "
    caches = [f"{c.removesuffix(' cache')} = {tex(base[c])}" for c in
              ("L1d cache", "L1i cache", "L2 cache", "L3 cache") if c in base]
    if caches:
        texto += "O sistema reportou " + ", ".join(caches) + ". "
    instrucoes = [v for t, v in (("avx", "AVX"), ("avx2", "AVX2"), ("avx512f", "AVX-512"), ("fma", "FMA"))
                  if t in meta.get("cpu_flags", ())]
    if instrucoes:
        texto += "Os recursos SIMD expostos incluem " + ", ".join(instrucoes) + ". "
    freq = meta.get("frequencias_khz", {})
    if freq.get("cpuinfo_min_freq") and freq.get("cpuinfo_max_freq"):
        texto += (f"A faixa de frequência reportada foi {decimal(int(freq['cpuinfo_min_freq']) / 1000, 0)} a "
                  f"{decimal(int(freq['cpuinfo_max_freq']) / 1000, 0)} MHz. ")
    if freq.get("scaling_governor"):
        texto += rf"O governador de frequência registrado foi \texttt{{{tex(freq['scaling_governor'])}}}. "
    if meta.get("hipervisor"):
        texto += (f"O ambiente expõe {len(meta['cpus_permitidas'])} CPUs lógicas e a indicação de hipervisor. "
                  "A hierarquia física de cache, a frequência "
                  "efetiva e as condições de isolamento ficam indeterminadas para esta coleta. ")
    if meta.get("likwid"):
        primeira = next((linha for linha in meta["likwid"].splitlines() if "version" in linha.lower()), "")
        if primeira:
            texto += f"O coletor registrou {tex(primeira)}."
    return texto


def explica(familia, regiao, grupos, dados):
    """Relaciona cada gráfico a valores observados e ao trabalho da região."""
    n = max(TAMANHOS)
    numeros = []
    for grupo in grupos:
        b = dados[grupo, regiao, "baseline", n]["media"]
        o = dados[grupo, regiao, "otimizada", n]["media"]
        prefixo = rf"Em \texttt{{{tex(grupo)}}}, a" if familia == "flops" else "A"
        numeros.append(f"{prefixo} baseline mediu {decimal(b)} e a otimizada, {decimal(o)}.")
    base = (f"Para $N={n}$, " + numeros[0][0].lower() + numeros[0][1:]
            + " " + " ".join(numeros[1:])).strip()

    if familia == "tempo":
        baixo = dados["tempo", regiao, "baseline", TAMANHOS[0]]["media"]
        baixo_opt = dados["tempo", regiao, "otimizada", TAMANHOS[0]]["media"]
        dobra = o / dados["tempo", regiao, "otimizada", 10000]["media"]
        fenomenos = {"Newton_Total": "A curva agrega resíduo, Jacobiana, resolução linear e atualização.",
                     "Jacobiana": "A menor região é sensível à resolução do relógio e ao custo de chamada.",
                     "Sistema_Linear": "A eliminação e a retrocessão mantêm dependências entre linhas vizinhas."}
        pior = max((dados["tempo", regiao, v, d]["desvio"] / dados["tempo", regiao, v, d]["media"], v, d)
                   for v in VERSOES for d in TAMANHOS)
        dispersao = ""
        if pior[0] >= 0.20:
            amostra = dados["tempo", regiao, pior[1], pior[2]]
            dispersao = (f" Em $N={pior[2]}$, a {pior[1]} teve média de {decimal(amostra['media'], 4)} ms "
                         f"e mediana de {decimal(amostra['mediana'], 4)} ms. "
                         f"O desvio-padrão equivaleu a {decimal(100 * pior[0], 0)}\\% da média.")
        return (base + f" Os valores estão em ms para 25 passos. Em $N={TAMANHOS[0]}$, as médias foram "
                f"{decimal(baixo, 4)} e {decimal(baixo_opt, 4)} ms, respectivamente. "
                f"O quociente de tempos baseline/otimizada em $N={n}$ foi {decimal(b / o, 2)}. "
                f"Na otimizada, dobrar $N$ de 10000 para 20000 multiplicou o tempo por {decimal(dobra, 2)}. "
                + fenomenos[regiao] + dispersao)
    if familia == "banda":
        fenomenos = {"Newton_Total": "O fluxo abrange as passagens por X, pelo resíduo e pela banda da Jacobiana.",
                     "Jacobiana": "A baseline escreve as três diagonais, enquanto a otimizada escreve só a principal.",
                     "Sistema_Linear": "O caminho de Thomas percorre a diagonal principal e o lado direito."}
        return base + " Os valores são taxas em MB/s. " + fenomenos[regiao] + " A taxa também depende da duração da região."
    if familia == "cache":
        fenomenos = {"Newton_Total": "A troca de buffers reutiliza as mesmas áreas entre passos.",
                     "Jacobiana": "As gravações das diagonais laterais ampliam o conjunto acessado pela baseline.",
                     "Sistema_Linear": "O percurso reverso reutiliza parte das linhas de cache acessadas na eliminação."}
        return (base + f" A diferença baseline menos otimizada foi {decimal(100 * (b - o), 2)} pontos percentuais. "
                + fenomenos[regiao] + " A taxa é relativa às referências contadas pelo grupo da CPU.")
    fenomenos = {"Newton_Total": "Resíduo, diagonal e atualização contêm laços candidatos a aritmética vetorial.",
                 "Jacobiana": "Cada derivada da diagonal principal pode ser calculada independentemente.",
                 "Sistema_Linear": "As recorrências de Thomas usam aritmética escalar. A verificação de dominância admite SIMD."}
    return base + " Os valores estão em MFLOP/s. " + fenomenos[regiao] + " Cada grupo mantém sua própria série."


def escreve_latex(pasta, destino, figuras, dados, meta):
    """Prepara as figuras e o texto quantitativo do artigo a partir da campanha."""
    destino.mkdir(parents=True, exist_ok=True)
    (destino / "figuras").mkdir(exist_ok=True)
    n = max(TAMANHOS)
    b = dados["tempo", "Newton_Total", "baseline", n]["media"]
    o = dados["tempo", "Newton_Total", "otimizada", n]["media"]
    grupos = set(meta["grupos_concluidos"])
    completo = all(grupos.intersection(alternativas) for alternativas in GRUPOS)
    piloto = meta.get("hipervisor") or not completo or not meta.get("topologia_likwid")
    qualificacao = " no ensaio preliminar" if piloto else " na campanha de desempenho"
    resumo = (f"Para $N={n}$, o tempo médio total passou de {decimal(b)} para {decimal(o)} ms"
              f"{qualificacao}, com razão de {decimal(b / o, 2)} entre os tempos.")
    if not completo:
        resumo += " A caracterização por contadores LIKWID está pendente."

    texto = ["% Gerado por benchmark/analise.py a partir das amostras registradas.",
             f"\\newcommand{{\\ConstanteUltima}}{{{meta['last_constant']}}}",
             f"\\newcommand{{\\Repeticoes}}{{{meta['repeticoes']}}}",
             f"\\newcommand{{\\EstadoRelatorio}}{{{'Ensaio preliminar' if piloto else 'Campanha de desempenho'}}}",
             f"\\newcommand{{\\ResumoMedido}}{{{resumo}}}",
             f"\\newcommand{{\\AmbienteMedido}}{{{descricao_ambiente(meta)}}}",
             r"\newcommand{\ResultadosMedidos}{%"]
    for familia, regiao, grupos_fig, nome in figuras:
        shutil.copyfile(pasta / "graficos" / nome, destino / "figuras" / nome)
        titulo = f"{TITULOS[familia]} para {REGIAO_PROSA[regiao]}."
        if familia == "tempo":
            titulo += " A faixa sombreada indica um desvio-padrão em torno da média."
        texto.extend([r"\begin{figure}[H]", r"\centering",
                      f"\\includegraphics[width=0.88\\linewidth]{{{nome}}}",
                      f"\\caption{{{titulo}}}", r"\end{figure}", explica(familia, regiao, grupos_fig, dados), ""])

    faltantes = [alternativas[0] for alternativas in GRUPOS if not grupos.intersection(alternativas)]
    substituicoes = [f"{alternativas[0]} por {grupo}" for alternativas in GRUPOS for grupo in alternativas[1:]
                     if grupo in grupos]
    if faltantes:
        nomes = ", ".join(r"\texttt{" + tex(g) + "}" for g in faltantes)
        texto.extend([r"\subsection{Contadores pendentes}",
                      f"A campanha ainda precisa de séries completas para {nomes}. ",
                      "As curvas correspondentes requerem a coleta dos contadores em um equipamento compatível."])
    if substituicoes:
        texto.append("A coleta registrou a substituição de " + tex(" e de ".join(substituicoes)) + ".")
    texto.append("}")
    (destino / "dados.tex").write_text("\n".join(texto) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("campanha", type=Path)
    parser.add_argument("--relatorio", type=Path, help="diretório das figuras e do trecho LaTeX")
    args = parser.parse_args()
    pasta = args.campanha.resolve()
    meta = json.loads((pasta / "campanha.json").read_text(encoding="utf-8"))
    dados = agrega(pasta, meta)
    if "tempo" not in meta["grupos_concluidos"]:
        raise ValueError("uma campanha de tempo completa é necessária")
    escreve_resumo(pasta, dados)
    graficos = pasta / "graficos"
    graficos.mkdir(exist_ok=True)
    figuras = []
    for familia, opcoes in FAMILIAS.items():
        grupos = [g for g in opcoes if g in meta["grupos_concluidos"]]
        if grupos:
            for regiao in REGIOES:
                nome = gera_figura(graficos, familia, regiao, grupos, dados, meta)
                figuras.append((familia, regiao, grupos, nome))
    if args.relatorio:
        escreve_latex(pasta, args.relatorio, figuras, dados, meta)
    print(f"{len(figuras)} gráficos gerados em {graficos}.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError) as erro:
        print(f"Análise: {erro}", file=sys.stderr)
        sys.exit(1)
