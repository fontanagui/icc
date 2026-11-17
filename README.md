# Broyden tridiagonal · CI1164 — versão simplificada

**Comece por [`GUIA_CODIGO.md`](GUIA_CODIGO.md).** Os arquivos C foram reescritos com funções e variáveis mais fáceis de acompanhar, inspirados na organização do projeto The Boys. As otimizações de alto desempenho foram preservadas.

O programa resolve o sistema não linear de Broyden com Newton em dupla precisão. A execução oficial usa `./broyden`. Cada atualização custa tempo e memória de trabalho proporcionais a `N`.

## Compilar, executar e testar

Requer Linux x86-64 com AVX, GCC e GNU Make. Python 3 é usado nos testes.

```bash
make
printf '32\n-1\n1e-12\n25\n' | ./broyden
make baseline
make test
make sanitize
```

A entrada é `N x0 epsilon MAX`, separada por espaços ou quebras de linha. `x0` é o valor inicial comum. Cada bloco de `stdout` contém `x1` a `xN`. Os blocos vêm separados por `#`. O rodapé contém o tempo total, o da Jacobiana e o da solução linear, em milissegundos. Um diagnóstico de falha é enviado a `stderr` com status de saída não zero.

O valor `epsilon=0` faz o laço executar até `MAX`, salvo falha numérica. Com `epsilon>0`, o programa aceita um iterado se `max(abs(F_i)) <= epsilon`. Com `MAX=0`, imprime o vetor inicial e três tempos iguais a zero. `make clean` remove executáveis e arquivos de compilação.

### Última equação

O padrão `LAST_CONSTANT=1` é a definição clássica, com `+1` na última equação. O gerador de expressões fornecido omite a constante nessa linha. A imagem do Moodle tem acesso restrito. Confira a última equação do enunciado antes de entregar. Se a figura usar a variante do gerador, compile e teste com `make test LAST_CONSTANT=0`, e inclua `LAST_CONSTANT=0` na coleta. Se essa for a variante exigida, defina `LAST_CONSTANT ?= 0` no Makefile antes de empacotar, para alinhar o `make` padrão ao corretor.

## Campanha com LIKWID

Use uma máquina física conforme o enunciado, com `likwid-perfctr`, `likwid-topology`, cabeçalhos do LIKWID e `liblikwid`. Matplotlib é necessário para as figuras e uma instalação LaTeX com `pdflatex` e Latin Modern para produzir o artigo. Em Linux Mint e Ubuntu, os pacotes `likwid`, `python3-matplotlib`, `texlive-latex-extra` e `lmodern` fornecem essas dependências nas distribuições que os disponibilizam.

```bash
likwid-topology -g -c
likwid-perfctr -a
make likwid
make benchmark BENCH_ARGS='--out resultados'
python3 benchmark/analise.py resultados --relatorio relatorio
make relatorio-pdf
```

Por padrão, a coleta escolhe a primeira CPU lógica da afinidade do processo. Para fixar outra CPU física apropriada, acrescente `--cpu ID` a `BENCH_ARGS` com o identificador lógico indicado por `likwid-topology`. O diretório informado em `--out` deve estar disponível para criação, para manter cada campanha em uma pasta própria. O coletor recompila com GCC na máquina de testes, fixa cada processo naquele núcleo, faz um aquecimento e coleta dez repetições para ambas as versões. Todos os resultados válidos usam `x0=-1`, `epsilon=0` e 25 atualizações. Use `--so-tempo` para executar apenas a coleta de tempos. A execução completa tenta as alternativas `L3`, `L1CACHE` e `CACHE` quando necessário e sinaliza grupos pendentes.

Para uma instalação fora dos caminhos padrão, informe `LIKWID_CPPFLAGS=-I/caminho/include` e `LIKWID_LDFLAGS=-L/caminho/lib` no Makefile ou no ambiente. Para confirmar a instrumentação isoladamente, esta invocação salva o relatório do LIKWID à parte da solução. O exemplo pressupõe a CPU lógica 0 e o grupo `FLOPS_DP` disponíveis.

```bash
printf '1000\n-1\n0\n25\n' | \
  BROYDEN_STATS=1 likwid-perfctr -C 0 -g FLOPS_DP -m -O \
  -o perfctr.csv ./broyden_likwid >/dev/null
```

As pastas `ambiente` e `bruto` da campanha registram topologia, flags, fórmulas de métricas e a saída original da coleta. `medicoes.csv` tem uma linha para cada repetição e região. `resumo.csv` registra média, mediana, desvio-padrão, mínimo e máximo. `campanha.json` indica quais grupos foram concluídos. Uma campanha com as quatro famílias de métricas produz doze gráficos. As legendas distinguem as versões e, nas curvas de FLOPS, os grupos DP e AVX.

Ao mudar flags de compilação ou LAST_CONSTANT, execute `make clean` antes de recompilar (o coletor usa `make -B`). Para reproduzir ensaios isolados de otimização, `FP_CONTRACT=off` desativa a contração FMA e `EXTRA_CPPFLAGS=-DBROYDEN_SEM_ALINHAMENTO` usa `malloc` convencional na otimizada. Use a mesma CPU e os mesmos parâmetros em todas as medições comparadas.

## Organização dos arquivos

| Arquivo ou pasta | Conteúdo |
|---|---|
| `main.c`, `io.c` | Entrada, geração da saída e código de retorno |
| `newton.c` | Espaço de trabalho, iterações, parada, temporizadores e marcadores |
| `broyden.c`, `jacobiana.c` | Resíduo, atualização e derivadas analíticas |
| `sistema_linear.c` | Thomas certificado e eliminação tridiagonal com pivotamento |
| `utils.c`, `utils.h` | Módulos fornecidos pela disciplina, incluindo `timestamp()` |
| `tests/` | Testes numéricos, da interface e dos formatos do LIKWID |
| `benchmark/` | Coleta, análise, geração de gráficos e empacotamento |
| `relatorio/` | Artigo editável em LaTeX e figuras da campanha incorporada |
| `DEFESA_TECNICA.md` | Roteiro para a defesa oral |
| `GUIA_CODIGO.md` | Explicação simples do fluxo e das otimizações |

## Fechar o artigo e o ZIP

O PDF incluído apresenta o ensaio preliminar de tempo em ambiente virtual. Os nove gráficos de hardware dependem da coleta em máquina compatível. Depois de rodar `benchmark/analise.py`, revise as figuras e a interpretação dos eventos no artigo. Preencha `\Autores` em `relatorio/artigo.tex` com os integrantes e os logins. Em seguida, recompile o PDF e passe seu login, ou os dois separados por hífen, para o empacotador. O empacotador usa a mesma base no ZIP, na pasta interna e no PDF interno.

```bash
make relatorio-pdf
python3 benchmark/empacota.py login1-login2 --dados resultados
```

Para trabalho individual, use `login1` no lugar de `login1-login2`.
