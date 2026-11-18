#!/usr/bin/env python3
"""Compila localmente e coleta tempos e métricas com repetições pareadas."""

import argparse
import csv
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import random
import re
import shlex
import shutil
import subprocess
import sys
from time import perf_counter_ns

from formato import CAMPOS, GRUPOS, PASSOS, REGIOES, TAMANHOS, VERSOES, le_likwid, le_tempos

PROJETO = Path(__file__).resolve().parents[1]
BINARIOS = {"baseline": "broyden_baseline", "otimizada": "broyden"}
AMBIENTE = {**os.environ, "LC_ALL": "C", "BROYDEN_STATS": "1"}


def registra(comando, log, obrigatorio=False):
    """Guarda os comandos e sua saída, inclusive em caso de falha."""
    try:
        resultado = subprocess.run(comando, cwd=PROJETO, env=AMBIENTE, text=True,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
        saida, status = resultado.stdout, resultado.returncode
    except (OSError, subprocess.TimeoutExpired) as erro:
        saida, status = f"{erro}\n", 1
    log.write_text(f"$ {shlex.join(comando)}\n{saida}\n[status {status}]\n", encoding="utf-8")
    if status and obrigatorio:
        raise RuntimeError(f"falha na compilação. Consulte {log}")
    return saida, status


def registra_ambiente(pasta, cpu):
    """Registra topologia, compilador, afinidade e política de frequência."""
    lscpu, _ = registra(["lscpu"], pasta / "lscpu.txt")
    registra(["cat", "/etc/os-release"], pasta / "sistema.txt")
    registra(["cat", "/proc/sys/kernel/perf_event_paranoid"], pasta / "perf-event-paranoid.txt")
    registra(["ls", "-ld", "/dev/cpu"], pasta / "acesso-msr.txt")
    registra(["lscpu", "-C"], pasta / "caches.txt")
    gcc, _ = registra(["gcc", "--version"], pasta / "gcc.txt")
    opcoes, _ = registra(["gcc", "-march=native", "-Q", "--help=target"], pasta / "gcc-alvo.txt")
    arquitetura = re.search(r"(?m)^\s*-march=\s+(\S+)", opcoes)
    cpuinfo = Path("/proc/cpuinfo").read_text(encoding="utf-8")
    blocos = cpuinfo.strip().split("\n\n")
    bloco = next((b for b in blocos if re.search(rf"^processor\s+:\s+{cpu}$", b, re.M)), blocos[0])
    (pasta / "cpuinfo.txt").write_text(bloco + "\n", encoding="utf-8")
    identificacao = dict((k.strip(), v.strip()) for k, v in
                         (linha.split(":", 1) for linha in bloco.splitlines() if ":" in linha))
    caminho_freq = Path(f"/sys/devices/system/cpu/cpu{cpu}/cpufreq")
    frequencias = {}
    for campo in ("scaling_governor", "scaling_driver", "cpuinfo_min_freq", "cpuinfo_max_freq"):
        try:
            frequencias[campo] = (caminho_freq / campo).read_text(encoding="utf-8").strip()
        except OSError:
            continue
    extras = dict((k.strip(), v.strip()) for k, v in
                  (linha.split(":", 1) for linha in lscpu.splitlines() if ":" in linha))
    return {"sistema": platform.platform(), "cpu_logica": cpu, "cpus_permitidas": sorted(os.sched_getaffinity(0)),
            "cpu_modelo": identificacao.get("model name", ""), "cpu_vendor": identificacao.get("vendor_id", ""),
            "cpu_familia": identificacao.get("cpu family", ""), "cpu_stepping": identificacao.get("stepping", ""),
            "alvo_gcc": arquitetura[1] if arquitetura else "",
            "cpu_flags": identificacao.get("flags", "").split(),
            "hipervisor": "hypervisor" in identificacao.get("flags", "").split(),
            "lscpu": extras, "frequencias_khz": frequencias, "gcc": gcc.splitlines()[0] if gcc else ""}


def mede(n, binario, grupo, cpu, base, metrica=None):
    """Descarta os vetores e conserva o stderr e o CSV nativo de contadores."""
    binario = str(binario)
    if grupo == "tempo":
        comando = ["taskset", "-c", str(cpu), binario]
    else:
        comando = ["likwid-perfctr", "-C", str(cpu), "-g", grupo, "-m", "-O",
                   "-o", str(base.with_suffix(".csv")), binario + "_likwid"]
    log = base.with_suffix(".stderr")
    inicio = perf_counter_ns()
    try:
        resultado = subprocess.run(comando, cwd=PROJETO, env=AMBIENTE, text=True,
                                   input=f"{n}\n-1\n0\n{PASSOS}\n", stdout=subprocess.DEVNULL,
                                   stderr=subprocess.PIPE, timeout=60)
    except (OSError, subprocess.TimeoutExpired) as erro:
        log.write_text(f"$ {shlex.join(comando)}\n{erro}\n", encoding="utf-8")
        raise ValueError(f"falha ao executar a amostra, consulte {log}") from erro
    parede = (perf_counter_ns() - inicio) / 1e6
    log.write_text(f"$ {shlex.join(comando)}\n{resultado.stderr}[status {resultado.returncode}]\n",
                   encoding="utf-8")
    if resultado.returncode != 0:
        raise ValueError(f"status {resultado.returncode}, consulte {log}")
    try:
        tempos = le_tempos(resultado.stderr)
        if grupo == "tempo":
            return {r: ("timestamp [ms]", tempos[r], PASSOS, "", tempos[r],
                        parede if r == REGIOES[0] else "") for r in REGIOES}
        dados = le_likwid(base.with_suffix(".csv"), grupo, cpu, metrica)
        return {r: (*dado, tempos[r], "") for r, dado in dados.items()}
    except (ValueError, OSError) as erro:
        raise ValueError(f"{erro}. Consulte {log}") from erro


def seleciona_grupos(pasta, cpu, sobrescritas, meta, binario):
    """Sonda grupos na ordem do enunciado e registra toda substituição."""
    topo, erro_topo = registra(["likwid-topology", "-g", "-c"], pasta / "topologia.txt")
    meta["topologia_likwid"] = topo if erro_topo == 0 else ""
    versao, erro_versao = registra(["likwid-topology", "-v"], pasta / "likwid.txt")
    meta["likwid"] = versao.strip() if erro_versao == 0 else ""
    lista, status = registra(["likwid-perfctr", "-a"], pasta / "grupos.txt")
    if status:
        meta["limitacoes"]["listagem"] = [f"Falha ao listar grupos. Consulte {pasta / 'grupos.txt'}."]
    disponiveis = set(re.findall(r"(?m)^\s*([A-Z][A-Z0-9_]*)[\t :-]+", lista)) if not status else None
    meta["grupos_disponiveis"] = sorted(disponiveis or ())
    escolhidos = []
    for alternativas in GRUPOS:
        tentativas = []
        for grupo in alternativas:
            if disponiveis is not None and grupo not in disponiveis:
                tentativas.append(f"{grupo} ausente na listagem")
                continue
            registra(["likwid-perfctr", "-H", "-g", grupo], pasta / f"grupo-{grupo}.txt")
            try:
                dados = mede(1000, binario, grupo, cpu, pasta / f"sonda-{grupo}", sobrescritas.get(grupo))
                escolhidos.append(grupo)
                meta["metricas_likwid"][grupo] = dados[REGIOES[0]][0]
                break
            except ValueError as erro:
                tentativas.append(f"{grupo}: {erro}")
        if tentativas:
            meta["limitacoes"][alternativas[0]] = tentativas
    return escolhidos


def pares(repeticao, versoes):
    """Embaralha N com semente fixa e reveza a ordem das versões."""
    tamanhos = list(TAMANHOS)
    random.Random(1164 + repeticao).shuffle(tamanhos)
    for i, n in enumerate(tamanhos):
        deslocamento = (i + repeticao) % len(versoes)
        ordem = versoes[deslocamento:] + versoes[:deslocamento]
        for versao in ordem:
            yield n, versao


def ensaio(grupo, args, pasta, escritor, metrica, binarios):
    """Descarta o aquecimento e registra uma linha por região em cada execução."""
    pasta.mkdir()
    for repeticao in range(args.repeticoes + 1):
        for n, versao in pares(repeticao, tuple(binarios)):
            base = pasta / f"r{repeticao:02d}-n{n}-{versao}"
            dados = mede(n, binarios[versao], grupo, args.cpu, base, metrica)
            if repeticao:
                for regiao, amostra in dados.items():
                    escritor.writerow((n, versao, repeticao, regiao, grupo, *amostra))


def argumentos():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=Path("resultados"))
    parser.add_argument("--cpu", type=int, default=min(os.sched_getaffinity(0)))
    parser.add_argument("--repeticoes", type=int, default=10)
    parser.add_argument("--last-constant", type=int, choices=(0, 1), default=1)
    parser.add_argument("--so-tempo", action="store_true", help="não mede contadores de hardware")
    parser.add_argument("--metrica", action="append", default=[], metavar="GRUPO=NOME")
    parser.add_argument("--original", type=Path, metavar="DIR",
                        help="recompila e inclui a baseline e a otimizada da fonte original")
    parser.add_argument("--binario", action="append", default=[], metavar="ROTULO=ARQUIVO",
                        help="inclui um executável intermediário já compilado, com --so-tempo")
    args = parser.parse_args()
    if args.cpu not in os.sched_getaffinity(0) or args.repeticoes < 1:
        parser.error("escolha uma CPU da afinidade atual e pelo menos uma repetição")
    if any("=" not in item for item in args.metrica):
        parser.error("use --metrica GRUPO=NOME para selecionar uma métrica da CPU")
    extras = {}
    for item in args.binario:
        nome, igual, caminho = item.partition("=")
        if (not args.so_tempo or not igual or not re.fullmatch(r"[a-z][a-z0-9-]*", nome)
                or nome in (*VERSOES, "baseline-original", "otimizada-original", *extras)
                or not Path(caminho).is_file() or not os.access(caminho, os.X_OK)):
            parser.error("use --so-tempo e --binario ROTULO=ARQUIVO executável com rótulo único")
        extras[nome] = Path(caminho).resolve()
    args.binario = extras
    if args.original:
        args.original = args.original.resolve()
        if args.original == PROJETO or not (args.original / "Makefile").is_file():
            parser.error("--original precisa apontar para a cópia separada dos fontes originais")
    args.out = args.out.resolve()
    return args


def main():
    args = argumentos()
    args.out.mkdir(parents=True, exist_ok=False)
    pasta = args.out / "ambiente"
    pasta.mkdir()
    bruto = args.out / "bruto"
    bruto.mkdir()
    meta = registra_ambiente(pasta, args.cpu)
    binarios = {v: PROJETO / b for v, b in BINARIOS.items()}
    if args.original:
        binarios.update({f"{v}-original": args.original / b for v, b in BINARIOS.items()})
    binarios.update(args.binario)
    meta["versoes"] = {v: str(b) for v, b in binarios.items()}
    meta["comando"] = shlex.join([sys.executable, *sys.argv])
    meta.update({"inicio_utc": datetime.now(timezone.utc).isoformat(), "dimensoes": TAMANHOS,
                 "x0": -1, "epsilon": 0, "max": PASSOS, "repeticoes": args.repeticoes,
                 "last_constant": args.last_constant, "limitacoes": {}, "metricas_likwid": {},
                 "grupos_concluidos": [], "so_tempo": args.so_tempo})
    make = ["make", "-B", f"LAST_CONSTANT={args.last_constant}"]
    compilacoes = [(make, "")]
    if args.original:
        compilacoes.append(([*make, "-C", str(args.original)], "-original"))
    metricas = dict(item.split("=", 1) for item in args.metrica)
    grupos = ["tempo"]
    try:
        for comando, sufixo in compilacoes:
            registra([*comando, "all", "baseline"], pasta / f"compilacao{sufixo}.txt", obrigatorio=True)
        if args.so_tempo:
            meta["limitacoes"]["LIKWID"] = ["Campanha executada no modo somente tempo."]
        elif not shutil.which("likwid-perfctr"):
            meta["limitacoes"]["LIKWID"] = ["likwid-perfctr ausente no PATH."]
        else:
            try:
                for comando, sufixo in compilacoes:
                    registra([*comando, "likwid"], pasta / f"compilacao-likwid{sufixo}.txt", obrigatorio=True)
                grupos += seleciona_grupos(pasta, args.cpu, metricas, meta, binarios["otimizada"])
            except RuntimeError as erro:
                meta["limitacoes"]["LIKWID"] = [str(erro)]

        with (args.out / "medicoes.csv").open("w", encoding="utf-8", newline="") as arquivo:
            escritor = csv.writer(arquivo)
            escritor.writerow(CAMPOS)
            for grupo in grupos:
                print(f"Coletando {grupo} na CPU lógica {args.cpu}...", flush=True)
                try:
                    ensaio(grupo, args, bruto / grupo, escritor, meta["metricas_likwid"].get(grupo), binarios)
                    meta["grupos_concluidos"].append(grupo)
                except ValueError as erro:
                    meta["limitacoes"].setdefault(grupo, []).append(str(erro))
                    print(f"{grupo}: {erro}", file=sys.stderr)
                arquivo.flush()
    finally:
        (args.out / "campanha.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2) + "\n",
                                                encoding="utf-8")
    print(f"Resultados em {args.out}. Grupos concluídos: {', '.join(meta['grupos_concluidos'])}.")
    completo = len(meta["grupos_concluidos"]) == len(GRUPOS) + 1 and bool(meta.get("topologia_likwid"))
    return 0 if completo or (args.so_tempo and meta["grupos_concluidos"] == ["tempo"]) else 2


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError) as erro:
        print(f"Coleta: {erro}", file=sys.stderr)
        sys.exit(1)
