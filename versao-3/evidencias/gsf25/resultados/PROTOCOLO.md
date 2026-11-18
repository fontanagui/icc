# Dados do experimento

As medições de 18 de novembro de 2026 usaram a mesma estação Lenovo
10MLS2B600 (Intel Core i5-7500) para a compilação e a execução. Ambas as
versões são monothread e foram fixadas na CPU lógica 3. Os parâmetros
foram `x0=-1` para todas as incógnitas, `epsilon=0` e `MAX=25`.
O [log de topologia](evidencias/topologia.txt) foi gerado com
`likwid-topology -g -c`.

| Arquivo | Conteúdo |
|---|---|
| `tempos_baseline.csv` | Três repetições por tamanho, N=32 até 2000 |
| `tempos_baseline_4000.csv` | Uma execução adicional da baseline, sem aquecimento |
| `tempos_otimizada.csv` | Três repetições para cada um dos 12 tamanhos |
| `likwid_pareado.csv` | Uma execução por grupo e versão, N=32 até 1000 |
| `likwid_baseline_2000.csv` | Uma execução por grupo na baseline em N=2000 |
| `likwid_otimizada.csv` | Uma execução por grupo na otimizada em N=2000, 4000, 8000, 9000, 10000 e 20000 |
| `resumo.csv` | Contagem, média, desvio-padrão amostral quando aplicável e indicadores de inconsistência |
| `graficos/` | 12 gráficos vetoriais em PDF |
| `evidencias/` | Topologia, grupos, fórmulas, versões, logs de compilação e testes |

## Tempo e contadores

A tabela de tempos mede o laço de Newton e acumula os tempos de cada
chamada das duas regiões internas via `timestamp()` em milissegundos.
A série de três repetições também executou um aquecimento por tamanho.
A terceira amostra de tempo da baseline em N=2000 coincidiu com parte
de uma coleta-piloto de hardware em outro núcleo.
A extensão em N=4000 levou 386,09 s e fornece um ponto sem
desvio-padrão. As dimensões de N=8000 a N=20000 da baseline estão
sem amostras de tempo.

O LIKWID 5.5.2 foi usado no modo marcador (`-m`), com o grupo e o
núcleo de cada execução indicados no CSV. Cada caminho em
`arquivo_likwid` é relativo a este diretório. O papel das métricas
está registrado no [log de fórmulas](evidencias/metricas.txt).

| Grupo | Métrica plotada |
|---|---|
| MEM | Memory bandwidth [MBytes/s] |
| L2CACHE | L2 miss ratio |
| FLOPS_DP | DP [MFLOP/s] |
| FLOPS_AVX | Packed DP [MFLOP/s] |

As duas métricas de FLOPS vêm de execuções independentes. A métrica
`AVX DP [MFLOP/s]` do grupo FLOPS_DP também foi preservada, sob o nome
`flops_dp_avx`, para comparações dentro da mesma execução.

Os valores de MEM foram mantidos no CSV e nos gráficos. O campo
`inconsistente_teto_mem` sinaliza taxas acima de 38.400 MB/s. Esse
teto nominal decorre da [especificação do processador](https://www.intel.com/content/www/us/en/products/sku/97123/intel-core-i57500-processor-6m-cache-up-to-3-80-ghz/specifications.html)
para dois canais DDR4-2400. O campo `inconsistente_volume_mem`
sinaliza quando a soma dos volumes das duas regiões internas excede
o volume de NEWTON, como na baseline em N=2000.

As combinações de hardware da baseline a partir de N=4000 estão sem
amostras. A matriz densa requer 3,2 GB em N=20000, acima da memória
disponível em uma sondagem durante a campanha. O crescimento cúbico
projetado implica horas por execução nos maiores tamanhos.

## Regeneração

A partir da raiz:

```sh
python3 scripts/plotar_tempos.py resultados/tempos_baseline.csv resultados/tempos_baseline_4000.csv \
    resultados/tempos_otimizada.csv \
    --likwid resultados/likwid_pareado.csv resultados/likwid_baseline_2000.csv resultados/likwid_otimizada.csv \
    --resumo resultados/resumo.csv --teto-mem 38400
pdflatex -halt-on-error relatorio-gsf25.tex
pdflatex -halt-on-error relatorio-gsf25.tex
```
