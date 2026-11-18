# Método de Newton para o sistema tridiagonal de Broyden

Trabalho de CI1164, Introdução à Computação Científica, da Universidade Federal do Paraná. Guilherme Fontana (gsf25) e Pedro Stahel de Azevedo (psa25, GRR20252084).

O programa compara três implementações da resolução linear no método de Newton. As três usam a mesma entrada, a mesma saída, os mesmos critérios de parada e o mesmo relógio.

| Executável | Implementação |
|---|---|
| `broyden` | Eliminação pelas extremidades, com pivoteamento em banda quando a condição dos pivôs falha |
| `broyden_baseline` | Eliminação em banda com pivoteamento parcial |
| `broyden_densa` | Eliminação de Gauss densa com pivoteamento parcial |

## Compilar e executar

Use GCC, GNU Make e Linux x86-64 com AVX. Os testes e a coleta usam Python 3. A geração de gráficos usa Matplotlib e a compilação do artigo usa pdfLaTeX com Latin Modern.

```sh
make all baseline densa
printf '32\n-1\n1e-12\n25\n' | ./broyden
make test LAST_CONSTANT=0
make test LAST_CONSTANT=1
```

A entrada é `N x0 epsilon MAX`, com os quatro valores separados por espaços ou quebras de linha. A saída mostra o vetor inicial e os vetores atualizados, com `#` entre eles. O rodapé mostra três tempos em milissegundos, na ordem cálculo de Newton, montagem da Jacobiana e resolução linear. Com tolerância positiva, a parada usa a norma infinito do resíduo. Com `epsilon=0`, o cálculo prossegue até `MAX` ou uma falha numérica.

`LAST_CONSTANT=0` segue o [gerador de referência](evidencias/gsf25/broyden.sh), que omite o termo constante da última equação. `LAST_CONSTANT=1` inclui o termo, como na formulação clássica. A figura oficial do enunciado ficou pendente de conferência. O Makefile recompila quando muda o valor da opção.

## Relatório e medições

- [Relatório final em PDF](relatorio-final.pdf) e [fonte LaTeX](relatorio/artigo.tex).
- [Campanha integrada](resultados/integrada/) com as amostras, estatísticas, razões entre médias, gráficos e logs. Foram medidas 10 repetições de 25 passos, com `x0=-1`, `epsilon=0` e `LAST_CONSTANT=0`. A referência densa foi medida até `N=1000` e as duas versões tridiagonais até `N=20000`.
- [Campanha com LIKWID 5.5.2](evidencias/gsf25/resultados/PROTOCOLO.md), com dados brutos e fontes correspondentes. As curvas descrevem a referência densa e a primeira versão tridiagonal com pivoteamento.
- [Ensaios complementares](evidencias/psa25/) com a comparação entre o código original e o revisado, estágios intermediários e sondas LIKWID. Nesses ensaios, a constante final foi mantida em 1.
- [Logs de validação](resultados/validacao/) para as três versões e as duas constantes. ASan e UBSan passaram com `ASAN_OPTIONS=detect_leaks=0`. O ambiente bloqueou a execução do LeakSanitizer.

Para reproduzir a coleta em um diretório novo e regenerar os gráficos:

```sh
make benchmark BENCH_ARGS='--out resultados/repeticao --so-tempo --max-n-densa 1000 --repeticoes 10'
python3 benchmark/analise.py resultados/repeticao
make relatorio-pdf
```

O alvo `relatorio-pdf` compila o artigo a partir das figuras das campanhas documentadas. Use `--cpu` na coleta para fixar a execução em uma CPU disponível. Use `--max-n-densa 0` para medir apenas as versões tridiagonais. Para uma coleta instrumentada, compile com `make likwid` e execute o benchmark sem `--so-tempo` em um ambiente com acesso aos contadores. A coleta integrada usou os relógios do programa e as sondas históricas documentam a restrição de acesso aos contadores no outro ambiente.

