#!/usr/bin/env python3
"""Gera um ZIP com os nomes de pasta e relatório exigidos pelo enunciado."""

import argparse
from pathlib import Path
import re
import tempfile
import zipfile

PROJETO = Path(__file__).resolve().parents[1]
NOME_PADRAO = "broyden-ci1164"
EXTENSOES = {".c", ".h", ".sh", ".py", ".md", ".tex", ".pdf"}


def prepara(nome, pdf, dados, caminho):
    """Escreve apenas fontes, documentação e dados de uma campanha escolhida."""
    caminho.parent.mkdir(parents=True, exist_ok=True)
    caminho_pdf = pdf.resolve()
    raiz = ((p, p.name) for p in sorted(PROJETO.iterdir())
            if p.is_file() and (p.suffix in {".c", ".h", ".md"} or p.name == "Makefile"))
    arquivos = list(raiz)
    for subpasta in ("benchmark", "tests", "relatorio"):
        for arquivo in sorted((PROJETO / subpasta).rglob("*")):
            if arquivo.is_file() and arquivo.suffix in EXTENSOES and arquivo.resolve() != caminho_pdf:
                arquivos.append((arquivo, arquivo.relative_to(PROJETO).as_posix()))
    if dados:
        for arquivo in sorted(dados.rglob("*")):
            if arquivo.is_file() and arquivo.suffix in {".csv", ".json", ".txt", ".stderr"}:
                arquivos.append((arquivo, "dados/" + arquivo.relative_to(dados).as_posix()))
    arquivos.append((pdf, f"relatorio-{nome}.pdf"))

    with tempfile.TemporaryDirectory(dir=caminho.parent) as temp:
        temporario = Path(temp) / caminho.name
        with zipfile.ZipFile(temporario, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as zipado:
            for arquivo, relativo in arquivos:
                zipado.write(arquivo, f"{nome}/{relativo}")
        with zipfile.ZipFile(temporario) as zipado:
            if zipado.testzip() is not None:
                raise RuntimeError("o ZIP não passou pela verificação de integridade")
        temporario.replace(caminho)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("nome", nargs="?", default=NOME_PADRAO)
    parser.add_argument("--pdf", type=Path, default=PROJETO / f"relatorio-{NOME_PADRAO}.pdf")
    parser.add_argument("--dados", type=Path)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    if not re.fullmatch(r"[a-zA-Z0-9_]+(?:-[a-zA-Z0-9_]+)?", args.nome):
        parser.error("informe um login ou dois logins separados por hífen")
    if not args.pdf.is_file():
        parser.error("PDF indisponível. Execute make relatorio-pdf ou informe --pdf.")
    if args.dados and not (args.dados / "campanha.json").is_file():
        parser.error("o diretório em --dados precisa conter campanha.json")
    caminho = (args.out or PROJETO.parent / f"{args.nome}.zip").resolve()
    prepara(args.nome, args.pdf, args.dados, caminho)
    print(caminho)


if __name__ == "__main__":
    main()
