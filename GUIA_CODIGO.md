# Guia de leitura — Broyden com Newton

**Este projeto resolve o sistema não linear de Broyden do enunciado.** Os arquivos de Newton e eliminação de Gauss fornecidos pelo professor serviram como referência para *nomes, separação de funções e estilo*. Não usamos o avaliador genérico `libmatheval` nem uma matriz quadrada, pois as equações aqui são específicas de Broyden.

## Ordem recomendada

1. **`resolucaoSNL.c`**: `main` lê `N`, `x0`, `epsilon` e `MAXIT` e chama `newton`.
2. **`NewtonSNL.c`**: faz as iterações de Newton. Em cada passagem calcula `F(X)`, a Jacobiana, o sistema linear e atualiza `X`.
3. **`SistNaoLinear.c`**: `calcF`, `calcJ`, `norma` e `atualizaX`.
4. **`EliminacaoGauss.c`**: `eliminacaoGauss`, `triang` e `retrossubs`, como na estrutura do professor, mas usando uma matriz tridiagonal.
5. **`Entrada.c`**: checa os quatro valores da entrada e imprime os resultados. É a parte de validação; não é o algoritmo de Newton.
6. **`utils.c` e `utils.h`**: utilitários fornecidos na disciplina, incluindo `timestamp()`.

## Qual é a função de Broyden?

A equação interior, isto é, para `1 < i < N`, é:

```
F[i] = ( 3 - 2 * X[i] ) * X[i] - X[i-1] - 2 * X[i+1] + 1
```

A primeira equação não tem `X[i-1]` e a última não tem `X[i+1]`. A Jacobiana é a matriz das derivadas de `F` e tem somente três diagonais:

- diagonal principal: `3 - 4 * X[i]`;
- diagonal de baixo: `-1`;
- diagonal de cima: `-2`.

**Atenção:** a forma clássica de Broyden tem `+1` também na última equação. O arquivo `broyden.sh` fornecido na atividade não tem esse termo. O padrão desta versão é `LAST_CONSTANT=1`. Confira a imagem oficial do enunciado; se ela confirmar a ausência do `+1`, use `make clean && make LAST_CONSTANT=0` e faça o mesmo na coleta.

## O que acontece em uma iteração?

1. `calcF( FX, X, n )` calcula as equações para a aproximação atual.
2. `calcJ( SL.principal, X, n )` calcula a diagonal principal da Jacobiana.
3. O programa troca `F(X)` por `-F(X)`.
4. `eliminacaoGauss( &SL )` resolve `J * S = -F(X)` e deixa o passo `S` no vetor `FX`.
5. `atualizaX( FX, X, n )` calcula `X + S`.
6. `NewtonSNL.c` troca os ponteiros `X` e `FX`, sem copiar cada elemento, e imprime o novo vetor.

O `newton` continua sendo Newton; **Broyden é a função que está sendo resolvida**, não o método quase-Newton de atualização de Jacobiana também chamado “método de Broyden”.

## Por que não guardar a matriz inteira?

O código do professor trabalha com matrizes genéricas `N x N`. Isso é útil para aprender Gauss, mas, para o problema específico, desperdiçaria memória e tempo. Com `N = 20.000`, uma Jacobiana completa em `double` ocupa aproximadamente 3,2 GB. Como só três diagonais têm valores diferentes de zero, o código guarda apenas vetores de tamanho `N`. O custo é `O(N)` por iteração, tanto para memória quanto para o cálculo principal.

## Quais otimizações foram mantidas dos slides?

- **Vetores contíguos**, com acesso sequencial: melhora o uso de cache.
- **Um único bloco de memória**, reservado uma vez: evita alocações dentro das iterações.
- **Alinhamento a 64 bytes** na versão otimizada: facilita operações vetoriais do processador.
- **`restrict`**: informa ao compilador que os vetores de entrada e saída não se sobrepõem nas funções indicadas.
- **Laço sem condições de borda** no meio de `calcF`: facilita a vetorização automática.
- **Jacobiana analítica**: usa `3 - 4 * X[i]`, sem calcular derivadas numéricas ou criar `N x N`.
- **Eliminação tridiagonal**: `gaussRapido` usa uma passagem de ida e uma de volta. Quando a condição para esse caminho não vale, `triang` e `retrossubs` fazem a versão com pivotamento.
- **Troca de ponteiros**, não cópia de 20.000 valores.
- **Compilação**: `-O3 -march=native -mavx -fopt-info-vec`.
- **LIKWID**: marcadores de Newton total, Jacobiana e sistema linear.

Há duas pequenas partes menos familiares: `aligned_alloc` (alocação alinhada) e o caminho `gaussRapido` (reduz divisões). Elas estão isoladas em funções curtas e explicadas nos comentários. O restante usa os mesmos laços, vetores, `if`, funções e `return` que aparecem nos exemplos do professor.

## Padrão de escrita

Foi adotado o estilo do projeto *The Boys*: comentários curtos, nomes descritivos, espaço dentro dos parênteses ( `if ( condicao )` ), chaves na mesma linha ( `if ( condicao ) {` ), sem chaves para `if` com uma única operação, linhas em branco entre verificações independentes e antes de `return` quando ele não é a única operação do `if`.

## Como executar

```bash
make
make test
printf '32\n-1\n0\n25\n' | ./broyden
```

O programa imprime o vetor inicial, o resultado de cada atualização e três tempos em milissegundos. `epsilon=0` executa todas as `MAXIT` atualizações, se não houver erro numérico. O arquivo `README.md` tem os comandos para compilar a versão básica, usar LIKWID e gerar gráficos.

O relatório e as medições preservadas na pasta `relatorio/` são históricos da versão anterior. Para atribuir esses resultados à versão reescrita, **é preciso repetir a coleta**.
