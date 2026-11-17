# Guia do código — Broyden explicado do jeito mais direto

Este projeto foi organizado para ser fácil de ler, com nomes em português e comentários curtos. O código matemático continua com as mesmas otimizações essenciais da versão anterior.

## Por onde começar a leitura

1. **`main.c`** — lê os quatro parâmetros e chama `executa_newton`.
2. **`newton.c`** — faz as iterações, mede os tempos e reutiliza a memória.
3. **`broyden.c`** — calcula a função do problema e atualiza o vetor X.
4. **`jacobiana.c`** — calcula a diagonal principal da Jacobiana.
5. **`sistema_linear.c`** — resolve o sistema linear. É a parte matemática mais trabalhosa.
6. **`io.c`** — lê a entrada e imprime os resultados no formato exigido.

Os arquivos `.h` apenas apresentam as estruturas e as funções de cada módulo. O arquivo `utils.c` é o fornecido pelo professor e contém `timestamp()`.

## Uma iteração em linguagem simples

Em cada passo:

1. `broyden_rhs()` calcula `-F(x)`.
2. `constroi_jacobiana()` calcula `3 - 4*x[i]` para cada elemento da diagonal.
3. `resolve_sistema()` encontra a correção `passo` que resolve `J*passo = -F(x)`.
4. `aplica_passo()` soma essa correção ao vetor atual.
5. `newton.c` troca os ponteiros dos vetores, sem copiar os 20.000 elementos.
6. O programa imprime o novo vetor e repete, até a condição de parada.

## Por que continua otimizado?

| Onde | O que foi mantido | Por que é rápido |
|---|---|---|
| `jacobiana.c` | Só três diagonais; a principal é calculada diretamente | Não monta matriz quadrada de N por N |
| `sistema_linear.c` | Thomas quando há garantia sobre os pivôs; Gauss tridiagonal com trocas nos outros casos | Custo O(N), sem eliminação densa |
| `broyden.c` | Primeira e última equações fora do laço principal | Evita `if` repetido e facilita AVX |
| `newton.c` | Um único bloco de memória, vetores alinhados a 64 bytes | Menos alocações e acessos mais regulares |
| `newton.c` | Troca de ponteiros | Evita copiar todo o vetor em cada iteração |
| `broyden.c` e `jacobiana.c` | `restrict`, dados sequenciais e alinhamento | O GCC pode vetorizar os laços |
| `sistema_linear.c` | Reuso de divisões no algoritmo de Thomas | Menos operações caras |
| `Makefile` | `-O3 -march=native -mavx -fopt-info-vec` | Otimização para a máquina de teste |
| `newton.c` | Marcadores de Newton, Jacobiana e Sistema Linear | Permite medir separadamente no LIKWID |

## As únicas partes que parecem mais técnicas

- **`restrict`**: garante ao compilador que dois vetores não ocupam o mesmo espaço de memória.
- **`__builtin_assume_aligned`**: avisa ao GCC que o vetor começa em um endereço alinhado.
- **`#ifdef BROYDEN_BASELINE`**: escolhe o código básico ou otimizado na compilação.
- **`LIKWID_MARKER_START/STOP`**: delimita o trecho que terá os contadores de hardware medidos.
- **Thomas**: elimina uma incógnita por vez usando o fato de a Jacobiana ter apenas três diagonais. A volta calcula as correções da última para a primeira.
- **Pivotamento**: quando o pivô pode dar problema, a versão alternativa troca linhas vizinhas em vez de dividir por um valor inadequado.

Esses recursos foram mantidos porque contribuem para desempenho, correção ou medição. Todo o restante foi escrito para facilitar a leitura.

## Como compilar e conferir

```bash
make
make test
make sanitize
printf '3\n-1\n0\n25\n' | ./broyden
```

Para comparar a implementação normal com a referência básica, use `make baseline`.

## Sobre os resultados antigos

O artigo e a pasta `dados/` vieram da versão anterior e foram preservados para **não perder as medições históricas**. A reescrita mantém o algoritmo e passou pelos testes, mas, para entregar o artigo como resultado final da versão simplificada, execute novamente a campanha de medições na máquina de testes, com LIKWID disponível.

**Atenção:** confira na figura oficial do Moodle se a última equação tem `+1`. O script `broyden.sh` omite esse termo. O código atual usa a formulação clássica (`LAST_CONSTANT=1`), e o Makefile permite testar a outra variante.
