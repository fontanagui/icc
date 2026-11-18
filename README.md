# Broyden tridiagonal · CI1164

O executável `broyden` resolve o sistema não linear tridiagonal de Broyden pelo método de Newton. Um passo calcula `b = −F(X)` e a diagonal analítica de `J(X)`, resolve `J(X) S = b` e soma `S` a `X`. O custo por passo e a memória de trabalho são O(N).

## Compilar e testar

Requer Linux x86-64 com AVX, GCC e GNU Make. Os testes da interface e os scripts de coleta usam Python 3.

```bash
make
make baseline
printf '32\n-1\n1e-12\n25\n' | ./broyden
make test LAST_CONSTANT=1
make test LAST_CONSTANT=0
make sanitize LAST_CONSTANT=1
```

A entrada tem quatro valores separados por espaços ou quebras de linha, na ordem `N x0 epsilon MAX`. O programa imprime o vetor inicial e o resultado de cada atualização em linhas `x1 = valor`, com `#` entre vetores. O rodapé contém o total computacional, o tempo da Jacobiana e o tempo da solução linear, em milissegundos. Diagnósticos ficam em `stderr`.

Com `epsilon > 0`, a parada usa `max(abs(F_i)) <= epsilon`, ou o limite de iterações. Com `epsilon = 0`, o laço executa até `MAX`, sujeito a falha numérica. Com `MAX = 0`, os três tempos são zero.

### Última equação

O primeiro commit preserva um [gerador sem a constante final](https://github.com/fontanagui/icc/blob/7841b6c/referencia/broyden.sh). A cópia local do gerador em Downloads contém `+1` na última equação. A figura oficial é necessária para decidir a variante da correção. O padrão provisório é `LAST_CONSTANT=1`, usado nos experimentos. `LAST_CONSTANT=0` seleciona a variante sem a constante. O Makefile recompila ao mudar a opção.

## Campanha reproduzível

A campanha usa `x0=-1`, `epsilon=0`, 25 passos, 12 tamanhos até 20.000 e dez repetições por tamanho e versão após um aquecimento. Use o mesmo valor de `LAST_CONSTANT` em todas as versões da comparação.

```bash
mkdir -p experimentos/fonte-original
tar -xzf experimentos/fonte-original-fbb6084.tar.gz -C experimentos/fonte-original

likwid-topology -g -c
likwid-perfctr -a
make likwid
make benchmark BENCH_ARGS='--out resultados/rodada --cpu 2 --original experimentos/fonte-original'
python3 benchmark/analise.py resultados/rodada --relatorio relatorio
make relatorio-pdf RELATORIO_PDF=resultados/rodada/artigo-broyden.pdf
```

Escolha em `--cpu` um ID da afinidade local. Omita a opção para usar o primeiro ID disponível. O diretório `--out` é novo a cada campanha. `--original` recompila os dois binários originais e intercala suas execuções com as revisadas. `--so-tempo` coleta apenas os relógios do programa e de parede. Para cabeçalhos e bibliotecas LIKWID fora dos caminhos padrão, configure `LIKWID_CPPFLAGS` e `LIKWID_LDFLAGS`. Com contadores indisponíveis, a campanha guarda o diagnóstico, coleta tempos e retorna 2. O relatório requer Matplotlib e uma instalação LaTeX com Latin Modern.

`medicoes.csv` contém as amostras. `resumo.csv` registra média, mediana, desvio-padrão, mínimo e máximo. `comparacao.csv` registra o speedup por região e dimensão. `parede_ms` mede o processo completo com a saída descartada, incluindo formatação e inicialização. `ambiente/` e `bruto/` contêm comandos, flags, topologia e o material bruto. Com os grupos previstos disponíveis, a análise produz 12 gráficos de tempo, banda, faltas e FLOPS. O gráfico adicional de speedup requer `--original`.

## Código e resultados

| Arquivo | Responsabilidade |
|---|---|
| `resolucaoSNL.c` e `Entrada.c` | Entrada, saída e status de execução |
| `NewtonSNL.c` | Alocação, iterações, parada, relógios e marcadores |
| `SistNaoLinear.c` | Lado direito, diagonal analítica, norma e atualização |
| `EliminacaoGauss.c` | Eliminação tridiagonal e retrossubstituição |
| `tests/` | Referência densa, resíduos, interface, kernels e parser LIKWID |
| `benchmark/` | Coleta e análise experimental |
| `relatorio/` | Fonte LaTeX e figuras |

O [guia do código](GUIA_CODIGO.md) segue a execução da `main` à solução. O [registro de otimizações](REGISTRO_OTIMIZACOES.md) explica os estágios intermediários e relaciona os resultados aos fontes. O [artigo da revisão](experimentos/campanha-final/artigo-revisao.pdf) usa a campanha desta máquina. Os dados anteriores estão em `dados/` e no PDF histórico da raiz.
