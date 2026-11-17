# Newton para Sistemas Não Lineares de Broyden

Programa em C para resolver o sistema de Broyden da imagem do enunciado usando
o Método de Newton. Há duas versões corretas para comparar o custo de usar
uma matriz cheia com o de explorar sua estrutura tridiagonal.

## Organização

```text
.
├── Makefile                 # Compila versões, executa testes e limpa produtos
├── README.md                # Este guia
├── otimizada/               # Fontes e Makefile da versão tridiagonal
├── baseline/                # Fontes e Makefile da versão densa
├── referencia/              # Material original recebido, com créditos preservados
├── scripts/
│   ├── gerar_entradas.sh     # Gera linhas de entrada no formato atual
│   ├── medir_tempos.py       # Coleta tempos internos em CSV
│   └── plotar_tempos.py      # Gera os três gráficos de tempo
└── tests/
    ├── test_broyden.py       # Correção, formato e casos de borda
    └── test_comparacao.py    # Compara as iterações das duas versões
```

Os arquivos de compilação ficam em `otimizada/build/` e `baseline/build/`.
O diretório `resultados/` é criado pelo coletor de tempos quando necessário.
As versões atuais não dependem de libmatheval. A pasta `referencia/` conserva
o programa genérico antigo e não participa da compilação principal.

## Requisitos

- GCC e GNU Make em Linux/x86 com suporte a AVX;
- Python 3 para testes e coleta de tempos;
- matplotlib para gerar gráficos de tempo;
- LIKWID com biblioteca, cabeçalhos e acesso aos contadores para os testes de hardware;
- `taskset` se usar a opção `--cpu` no coletor.

GCC compila ambas as versões com:

```text
-O3 -march=native -mavx -fopt-info-vec -std=c11 -Wall -Wextra -Wpedantic
```

Compile na própria máquina usada nos experimentos. `-march=native` especializa
o executável para a CPU local. Mensagens `optimized: loop vectorized` são
diagnósticos esperados do compilador.

## Compilar e executar

Execute os comandos abaixo na raiz do projeto.

```sh
# Compila a versão otimizada e disponibiliza ./broyden, conforme o enunciado.
make
printf '32 -1 0 25\n' | ./broyden

# Compila as duas versões.
make versoes
printf '32 -1 0 25\n' | ./otimizada/broyden
printf '32 -1 0 25\n' | ./baseline/broyden
```

`./broyden` é uma cópia do executável otimizado, criada pelo Makefile principal.
Também é possível trabalhar dentro de cada pasta com `make`, `./broyden`,
`make test` e `make clean`.

### Entrada

Cada execução lê **um único caso** da entrada padrão:

```text
N x0 epsilon MAX
```

Os valores podem ser separados por espaços ou quebras de linha.

| Parâmetro | Significado | Valores aceitos |
|---|---|---|
| N | Número de incógnitas | Inteiro entre 2 e INT_MAX, sujeito à memória disponível |
| x0 | Mesmo valor inicial para todas as incógnitas | Número finito |
| epsilon | Tolerância absoluta de parada | Número finito maior ou igual a zero |
| MAX | Máximo de atualizações de Newton | Inteiro entre 0 e INT_MAX |

`epsilon=0` desativa a parada por tolerância. Sem falhas numéricas, são
executadas exatamente MAX atualizações. Para os experimentos do enunciado,
use `epsilon=0` e `MAX=25`, mantendo o mesmo x0 nas duas versões; os exemplos
usam x0=-1. `MAX=0` imprime somente o estado inicial.

Para gerar os casos exigidos:

```sh
bash scripts/gerar_entradas.sh
# Ou somente alguns tamanhos:
bash scripts/gerar_entradas.sh 32 64 128
```

O script imprime uma linha por caso. Como o executável lê um caso por
invocação, execute cada linha separadamente ou use o coletor de tempos.
Os exemplos em `referencia/sist.dat` pertencem ao formato genérico antigo.

### Saída

São impressos o estado inicial (iteração 0) e cada estado após uma atualização.
Cada incógnita aparece como `x1 = valor`, `x2 = valor`, etc., em linhas
separadas. Uma linha `#` separa estados. Com MAX=25 e epsilon=0, são 26 estados.

Ao final, entre duas linhas `###########`, aparecem três números em ms:

1. Tempo total do laço de Newton;
2. Soma dos tempos de construção das Jacobianas;
3. Soma dos tempos de resolução dos sistemas lineares.

Os tempos usam `timestamp()` de `utils.c`, com `CLOCK_MONOTONIC_RAW`.
Alocação, inicialização e impressão ficam fora da região medida. O histórico
é copiado dentro do laço e impresso depois, evitando que printf domine os
tempos. Erros de entrada, alocação, singularidade ou valores não finitos são
reportados em stderr, com código de saída diferente de zero.

## Diferenças entre as versões

| Característica | `otimizada/` | `baseline/` |
|---|---|---|
| Jacobiana | Quatro diagonais contíguas, incluindo preenchimento do pivoteamento | Matriz cheia contígua N × N, com ponteiros de linhas |
| Construção da Jacobiana | O(N) | O(N²), preenche também os zeros |
| Resolução linear | Gauss tridiagonal com pivoteamento parcial entre linhas adjacentes | Gauss genérico com pivoteamento parcial em toda a coluna |
| Custo da resolução | O(N) por iteração | O(N³) por iteração |
| Espaço da matriz | O(N) | O(N²) |
| Entrada, saída e critérios | Iguais | Iguais |
| Opções de compilação e marcadores | Iguais | Iguais |

A baseline percorre toda a submatriz na eliminação e todos os elementos
restantes na substituição regressiva. A otimizada evita armazenar e operar
sobre zeros estruturais. Ambas calculam as derivadas analiticamente e
reutilizam os buffers; o cálculo de F e a rotina de Newton são iguais.
Não há atrasos artificiais na baseline.

A comparação avalia principalmente algoritmo e estrutura de dados. Não se
pode atribuir todo o ganho a AVX: o compilador pode vetorizar laços nas duas
versões. A recorrência da eliminação tridiagonal continua sequencial.

**Tamanhos grandes na baseline:** para N=20000, só a matriz ocupa
3.200.000.000 bytes (aproximadamente 2,98 GiB). O custo cúbico pode tornar
as 25 iterações muito demoradas. Os testes rápidos usam tamanhos menores;
isso não substitui a série completa exigida para a análise do trabalho.

Nas duas versões, o histórico consome O((MAX+1)×N) doubles, além do espaço
numérico: cerca de 4,16 MB para N=20000 e MAX=25.

### Fórmula e critérios numéricos

Com índices de 1 a N, a função implementada é:

```text
f1 = (3 - 2*x1)*x1 - 2*x2 + 1
fi = (3 - 2*xi)*xi - x(i-1) - 2*x(i+1) + 1,  2 <= i < N
fN = (3 - 2*xN)*xN - x(N-1)
```

A última equação não tem o termo `+1`, conforme a imagem fornecida.
A Jacobiana tem diagonal `3 - 4*xi`, subdiagonal `-1` e superdiagonal `-2`.
A fórmula fornecida requer N>=2.

A cada atualização, resolve-se `J(x)*s = -F(x)` e calcula-se `x = x + s`.
Para epsilon>0, a execução para se a norma infinito de F for menor ou igual
à tolerância antes da resolução, ou se a norma infinito do passo for menor
ou igual à tolerância depois da atualização. O critério do passo isolado
não garante resíduo pequeno. Atingir MAX é uma parada normal e também não
garante convergência.

### Módulos em cada versão

| Arquivo | Responsabilidade |
|---|---|
| `resolucaoSNL.c` | Apenas main: entrada, validação e coordenação |
| `NewtonSNL.*` | Newton, histórico, critérios e impressão |
| `SistNaoLinear.*` | F, Jacobiana e norma infinito |
| `SistLinear.*` | Representação, alocação e liberação da matriz |
| `EliminacaoGauss.*` | Resolução linear com pivoteamento |
| `utils.*` | Timestamp fornecido e funções auxiliares |
| `marcadores.h` | Instrumentação LIKWID opcional |

## Testes

```sh
make test
```

Esse comando compila as duas versões e executa:

- Na otimizada: os 12 tamanhos exigidos até N=20000, com x0=-1,
  epsilon=0 e MAX=25, verificando resíduo final inferior a 1e-11;
- Na baseline: casos até N=256, com os mesmos parâmetros;
- Comparação de todos os estados entre versões até N=512, com tolerância
  absoluta/relativa de 1e-10, sem comparar os tempos;
- Comparação com Gauss denso independente em Python, casos com pivô zero,
  singularidade, tolerância positiva, MAX=0 e entradas inválidas.

Para testar uma versão isoladamente:

```sh
make -C otimizada test
make -C baseline test
```

## Coleta de tempos e gráficos

Exemplo inicial com os mesmos tamanhos nas duas versões:

```sh
make versoes
python3 scripts/medir_tempos.py --exe ./otimizada/broyden --versao otimizada --tamanhos 32 64 128 256 512 --saida resultados/otimizada.csv
python3 scripts/medir_tempos.py --exe ./baseline/broyden --versao baseline_densa --tamanhos 32 64 128 256 512 --saida resultados/baseline.csv
python3 scripts/plotar_tempos.py resultados/otimizada.csv resultados/baseline.csv
```

O coletor faz um aquecimento por tamanho e dez repetições por padrão. Use
`--repeticoes 20` para alterar a quantidade e `--cpu NUMERO` para fixar a
execução a um núcleo disponível via taskset. Mantenha o mesmo núcleo e
as mesmas condições nas duas versões. Os tempos são os internos do programa,
sem a impressão. Sem `--tamanhos`, o coletor usa todos os 12 tamanhos exigidos.

Os três gráficos PDF ficam em `resultados/graficos/`. Cada um mostra uma
região, com cores distintas por versão e médias aritméticas por N. Os eixos
N e tempo são logarítmicos. A geração requer matplotlib.

## LIKWID

Para instrumentar ambas as versões:

```sh
make versoes LIKWID=1
# Em instalação fora dos caminhos padrão:
# make versoes LIKWID=1 LIKWID_INC=/caminho/include LIKWID_LIB=/caminho/lib

likwid-topology -g -c > topologia.txt
likwid-perfctr -a
printf '32 -1 0 25\n' | likwid-perfctr -C 0 -g MEM -m ./otimizada/broyden
printf '32 -1 0 25\n' | likwid-perfctr -C 0 -g MEM -m ./baseline/broyden
```

Troque o núcleo 0 por um disponível. Marcadores em ambas as versões:

| Marcador | Região |
|---|---|
| NEWTON | Laço completo de Newton |
| JACOBIANA | Construção da Jacobiana, acumulada entre iterações |
| SISTEMA_LINEAR | Resolução linear, acumulada entre iterações |

Os marcadores internos estão contidos em NEWTON: não some suas métricas ao
total como se fossem regiões disjuntas. O tempo total inclui F, normas,
atualizações, cópias do histórico e instrumentação dos trechos internos.
A sobrecarga de timestamp e marcadores pode ser significativa para N pequeno.

O enunciado exige banda de memória (MEM, ou L3), cache miss L2
(L2CACHE, ou alternativa L1 permitida) e operações aritméticas
(FLOPS_DP/FLOPS_AVX). Consulte os grupos e métricas disponíveis na CPU local,
documentando substituições. Use os resultados por marcador, porque o
processo completo também imprime o histórico.

Para voltar à compilação sem LIKWID, use `make versoes LIKWID=0`.

## Limpeza e situação da entrega

```sh
make clean
```

Remove executáveis, objetos, dependências de compilação e caches Python das
duas versões. Preserva fontes, referência e resultados experimentais.
Os produtos de compilação, caches e resultados locais estão no `.gitignore`.

A implementação e os testes estão disponíveis. Ainda faltam as medições
LIKWID reais, a análise experimental completa, os nove gráficos de hardware,
os três gráficos de tempo com dados finais, o artigo em PDF e o pacote com
os logins dos autores. Neste ambiente, LIKWID e matplotlib não estão
instalados; não foram produzidos resultados fictícios.

Preserve os créditos do material fornecido e descreva no artigo as mudanças
realizadas pelo grupo. O pacote final deve conter o Makefile principal,
as duas versões e os scripts necessários, além do relatório com o nome
exigido pelo enunciado.
