# Registro de otimizações · Broyden CI1164

## Ponto de partida e escopo

A versão original é o commit `fbb6084779fb17f1c6281e91d426ede16ed917ac`. Os fontes, o Makefile, os testes e os scripts daquela versão foram preservados em [fonte-original-fbb6084.tar.gz](experimentos/fonte-original-fbb6084.tar.gz). Os [testes iniciais](experimentos/verificacao-inicial/testes.log) e as rodadas [antes](experimentos/antes/) e [antes com a sonda LIKWID](experimentos/antes-likwid/) foram executados antes da primeira edição do solver. A rodada inicial com LIKWID produziu apenas tempos do relógio do programa. Os diagnósticos por grupo estão em [verificação inicial](experimentos/verificacao-inicial/).

Foram lidos os arquivos originais de Newton, Gauss, sistema linear e sistema não linear do professor em Downloads, os exemplos de otimização de multiplicação de matrizes e seus scripts de coleta. O estilo foi conferido no [código de TheBoysUFPR](https://github.com/PedroStahel/TheBoysUFPR/blob/main/fprio.c). O PDF `09-OtimizaçãoBásica.pdf` e a figura oficial do enunciado seguem pendentes. A quantidade de gráficos e o formato oficial de entrada e saída seguem sujeitos à conferência no enunciado. Os testes verificam o formato atual e as regras documentadas no projeto.

O `referencia/broyden.sh` do commit `7841b6c` omite a constante final. A cópia de Downloads tem `+1`. Isso afeta o valor de `F_N`, mas a derivada e o algoritmo linear são os mesmos nas duas variantes. O padrão anterior de `LAST_CONSTANT=1` foi adotado provisoriamente na rodada de desempenho. As duas definições foram testadas.

## Alterações medidas

As médias abaixo são da [rodada de estágios intercalados](experimentos/ensaios-isolados/resumo.csv), com N = 20.000, dez repetições na CPU lógica 2 e 25 passos por repetição. Cada ensaio comparou os estágios na mesma rodada. Os patches de desenvolvimento ficam em [estágios](experimentos/estagios/).

| Arquivo e função | Antes → depois | Conceito e impacto esperado | Resultado isolado |
|---|---|---|---|
| `SistNaoLinear.c`, `calcMenosF`, e `NewtonSNL.c`, `calculaPasso` | Calculava `F` e invertia os sinais em outro laço. Agora a expressão já escreve `−F`. | Fusão de operações e menor tráfego sobre o vetor. O laço otimizado trata as bordas separadamente e continua vetorizável. | Newton otimizado: 4,637 → 4,630 ms. Na básica: 9,641 → 9,727 ms. O efeito sobre o tempo total ficou pequeno diante da dispersão. A simplificação elimina um laço. |
| `EliminacaoGauss.c`, `gaussRapido` | A eliminação avançava em uma cadeia por toda a matriz. Agora avança das duas extremidades, resolve a linha central e volta às extremidades. | Duas sequências independentes permitem ao núcleo sobrepor parte da latência das divisões escalares. O algoritmo reaproveita os coeficientes normalizados e mantém uma divisão por linha. | Região linear: 3,538 → 1,986 ms (1,78×). Newton: 4,630 → 3,061 ms. |
| `EliminacaoGauss.c`, `triang` e `retrossubs`, e `SistLinear.h` | Inicializava a subdiagonal e lia cada `−1` do vetor. A retrocessão repetia checagens dos pivôs. Agora usa a constante e escreve a segunda superdiagonal conforme as trocas. | Remoção de leituras, inicialização e verificações redundantes. O pivô escolhido nas linhas anteriores é finito e tem módulo ≥ 1. A divisão direta do último pivô aceita um subnormal quando o resultado é finito. | Região linear básica: 8,231 → 6,823 ms (1,21×). Newton básico: 9,659 → 8,252 ms. |
| `NewtonSNL.c`, `alocaVetores`, e `SistNaoLinear.c`, `calcJ` e `atualizaX` | Teste com `malloc` e sem informar alinhamento ao GCC. A revisão manteve as fatias alinhadas a 64 bytes e o contrato de alinhamento interno. | Alinhamento de memória para SIMD. Os dois buffers de cada chamada são disjuntos e continuam alinhados após a troca. | Jacobiana: 0,304 → 0,406 ms ao remover alinhamento. Newton: 2,906 → 3,188 ms, com dispersão de 0,31 e 0,48 ms. O teste sem alinhamento foi descartado. |

A justificativa da eliminação pelas duas pontas está no [roteiro de defesa](DEFESA_TECNICA.md). A garantia dos pivôs vem de `|d[i]| >= 3`. Cada ponta desconta no máximo 1 no módulo do pivô ajustado. A linha central recebe dois descontos e ainda tem módulo pelo menos 1. O [log de compilação](experimentos/campanha-final/ambiente/compilacao.txt) registra vetorização de 32 e 16 bytes em laços independentes. O [assembly do sistema linear](experimentos/verificacao-final/gauss-avx.txt) permite localizar as divisões escalares nas duas cadeias.

### Demais correções

| Arquivo e função | Antes → depois | Motivo e evidência |
|---|---|---|
| `NewtonSNL.c`, `calculaPasso` e `newton` | Códigos numéricos e dois argumentos para o mesmo vetor. Agora usa estados com nomes e acessa o buffer pelo sistema linear. | O fluxo distingue avanço, convergência, pivô inválido e valor não finito. O impacto isolado sobre tempo não foi medido. |
| `Makefile` | Alterar uma opção podia deixar objetos de uma configuração anterior. A configuração agora participa das dependências. | Testar `LAST_CONSTANT=1` e depois `LAST_CONSTANT=0` funciona na mesma árvore. O alvo `sanitize` instrumenta também os kernels e permite configurar LeakSanitizer. O destino configurável do PDF permite preservar o relatório histórico. |
| `tests/test_kernels.c` e `tests/test_solver.py` | A Jacobiana podia passar com uma derivada não finita. O teste dos kernels só usava uma dimensão. Agora confere finitude, ordem 1–5, troca com pivô pequeno, determinante zero, pivô subnormal e fronteira de tolerância. | Testa a resolução da linha central em dimensões pares, ímpares e unitária. A referência independente de Gauss denso e os testes de saída, memória e resíduos continuam ativos. |
| `benchmark/coleta.py`, `formato.py` e `analise.py` | A coleta comparava só dois binários e, se a listagem falhasse, indicava grupos ausentes. Agora intercala as versões, guarda as sondas com seu status, registra o tempo de parede e produz a comparação de médias. | Mesmo problema, afinidade, relógio e flags para a comparação direta. Os tempos do processo completo ajudam a interpretar o impacto da formatação. |
| `README.md`, `GUIA_CODIGO.md`, `DEFESA_TECNICA.md`, `AVISO_RELATORIO.md` e `relatorio/` | Descreviam o caminho anterior e o artigo usava dados históricos. Agora há um guia da revisão e um artigo gerado com a rodada intercalada atual. | Registra a divergência das fontes, a atribuição de custos por região e as limitações de hardware. |

## Antes × depois

Os valores abaixo são da [campanha final intercalada](experimentos/campanha-final/), para N = 20.000 e 25 passos. Tempo em ms, expresso como média ± desvio-padrão amostral. O speedup é média antes dividida pela média depois.

| Região | Básica original | Básica revisada | Speedup básica | Otimizada original | Otimizada revisada | Speedup otimizada |
|---|---:|---:|---:|---:|---:|---:|
| Newton | 9,902 ± 0,414 | 8,209 ± 0,455 | 1,21× | 4,654 ± 0,396 | 3,121 ± 0,391 | 1,49× |
| Jacobiana | 0,386 ± 0,062 | 0,346 ± 0,067 | 1,11× | 0,312 ± 0,085 | 0,360 ± 0,111 | 0,87× |
| Sistema linear | 8,292 ± 0,228 | 6,843 ± 0,247 | 1,21× | 3,498 ± 0,117 | 2,003 ± 0,144 | 1,75× |

Entre básica e otimizada revisadas, o speedup total foi **2,63×**. O sistema linear caiu de 75% para 64% do total computacional da otimizada. A média da Jacobiana otimizada aumentou 15%, embora `calcJ` tenha o mesmo código. A dispersão e as diferenças de estado do cache entre os passos limitam a atribuição dessa variação. Em N = 1.000, a básica revisada teve regressão na média total (0,459 → 0,502 ms) e uma pequena melhora na mediana (0,452 → 0,444 ms), com desvio-padrão de 0,196 ms depois da revisão.

O tempo de parede inclui a criação do processo, leitura, cálculo e formatação. Para N = 20.000, suas medianas na otimizada foram 131,21 ms antes e 131,51 ms depois. Cada execução formata 520 mil linhas de variáveis mesmo com descarte em `/dev/null`. O ganho observado se concentra no tempo computacional.

A otimizada acessa três vetores no caminho rápido, com 24 N bytes de dados distintos. A básica usa os cinco vetores, com cerca de 40 N bytes. Com N = 20.000, são cerca de 469 KiB e 781 KiB, respectivamente. Ambos excedem L2 e cabem em L3 em capacidade nominal. Esta é uma estimativa de conjunto de trabalho, com faltas e tráfego efetivo ainda pendentes.

- [Amostras](experimentos/campanha-final/medicoes.csv), [estatísticas](experimentos/campanha-final/resumo.csv) e [speedups](experimentos/campanha-final/comparacao.csv).
- [Gráficos de Newton](experimentos/campanha-final/graficos/tempo-newton_total.pdf), [Jacobiana](experimentos/campanha-final/graficos/tempo-jacobiana.pdf), [sistema linear](experimentos/campanha-final/graficos/tempo-sistema_linear.pdf) e [speedup](experimentos/campanha-final/graficos/speedup-revisao.pdf).
- [Artigo em PDF](experimentos/campanha-final/artigo-revisao.pdf) e [logs brutos da rodada](experimentos/campanha-final/bruto.tar.gz).

## Hardware e LIKWID

Linux Mint Debian Edition 7, kernel 6.12.107, GCC 14.2.0 e Intel Core i5-7500, com quatro núcleos e quatro threads. A Intel nomeia o modelo Kaby Lake. O `likwid-topology` exibe a classificação “Intel Coffeelake processor”. Foram registrados L1d/L1i de 32 KiB por núcleo, L2 de 256 KiB por núcleo e L3 de 6 MiB compartilhado. A faixa reportada foi 800–3.800 MHz, com governador `powersave`. A medição fixou a CPU 2 via `taskset` para os tempos e solicitou a mesma CPU com `likwid-perfctr -C 2` para contadores. Afinidade evita migração, mas o sistema permaneceu compartilhado e a frequência ativa pode variar. [Topologia registrada](experimentos/campanha-final/ambiente/topologia.txt) · [Especificações da Intel](https://www.intel.com/content/www/us/en/products/sku/97123/intel-core-i57500-processor-6m-cache-up-to-3-80-ghz/specifications.html).

A cópia portátil foi extraída dos pacotes Debian LIKWID 5.4.1 e Lua 5.3.6, sem instalação no sistema. O LIKWID leu a topologia e o Makefile compilou as quatro variantes instrumentadas. `likwid-perfctr -a` e as sondas de grupo retornaram `Cannot use performance monitoring with perf_event_paranoid = 3`. `/dev/cpu` também estava ausente para acesso direto a MSRs. Os eventos a seguir são definições do pacote, com todos os valores medidos pendentes.

| Grupo sondado | Eventos ou métrica prevista | Unidade e interpretação |
|---|---|---|
| `FLOPS_DP` | `FP_ARITH_INST_RETIRED` escalar, DP 128 e 256 bits | MFLOP/s. A fórmula pondera 1, 2 ou 4 lanes por evento. |
| `FLOPS_AVX` | DP empacotado de 256 bits | MFLOP/s. Série vetorial do pacote, comparada separadamente. |
| `L2CACHE` | `L2_RQSTS_MISS / L2_TRANS_ALL_REQUESTS` | Fração de solicitações L2, conforme os eventos do grupo. |
| `MEM` | `DRAM_READS` e `DRAM_WRITES` | MB/s e GB nas fórmulas de banda e volume. Contadores do controlador de memória. |
| `L3` | `L2_LINES_IN_ALL` e `L2_TRANS_L2_WB` | MB/s. Tráfego entre níveis de cache. |
| `L1CACHE`, `CACHE` | Alternativas solicitadas para taxa de faltas | Disponibilidade efetiva pendente. |

O [diretório de ambiente](experimentos/campanha-final/ambiente/) contém saídas nativas e comandos. As [definições do pacote](experimentos/verificacao-inicial/grupos-do-pacote/) documentam as fórmulas consultadas. A grade planejada tem 12 figuras, com três regiões para cada uma das famílias tempo, banda, faltas de cache e FLOPS. A rodada produziu os três gráficos de tempo e o gráfico adicional de speedup. A contagem oficial depende do enunciado.

## Testes e reprodução

O original passou em `make test` para a variante clássica. O LeakSanitizer falhou ao iniciar neste ambiente. A suíte original passou com ASan e UBSan usando `ASAN_OPTIONS=detect_leaks=0`. Os estágios 1–4 passaram com a mesma suíte de referência e os kernels correspondentes. A revisão passou em `make test` e `make sanitize ASAN_OPTIONS=detect_leaks=0` para as duas constantes finais, nas versões básica e otimizada. Os testes regulares mediram resíduo final máximo de `1,11e-15` nos ensaios de 25 passos. O [diretório de verificação final](experimentos/verificacao-final/) guarda os logs.

Use os [comandos do README](README.md) para reconstruir a cópia original, testar e repetir a campanha. As flags da coleta foram `-std=c11 -Wall -Wextra -O3 -march=native -mavx -fopt-info-vec -ffp-contract=fast`. Os logs registram opções por binário. Cada rodada de tempos tem um aquecimento seguido de dez repetições com ordem alternada, mesmos 12 tamanhos e entrada `x0=-1`, `epsilon=0`, `MAX=25`. Isso controla o número de passos. Os testes com tolerância positiva tratam a convergência separadamente.
