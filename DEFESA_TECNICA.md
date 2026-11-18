# Roteiro de defesa

## Sistema e método

Para cada equação, com vizinhos de contorno igual a zero,

```text
F[i] = (3 − 2 X[i]) X[i] − X[i−1] − 2 X[i+1] + c[i]
J[i,i−1] = −1        J[i,i] = 3 − 4 X[i]        J[i,i+1] = −2
```

Newton recalcula essa Jacobiana analítica no ponto atual, resolve `J S = −F(X)` e atualiza `X`. Com tolerância positiva, o critério usa a maior discrepância entre as equações. Um passo pequeno, isoladamente, também pode ocorrer por estagnação.

## Eliminação

Na versão básica, há só um candidato a troca de linha abaixo da diagonal. Esse coeficiente é sempre `−1` na coluna que está sendo eliminada. Comparar com seu módulo garante que cada pivô intermediário seja finito e tenha módulo pelo menos 1. Uma troca alcança uma coluna a mais à direita. A última linha fornece o único pivô ainda sujeito a zero após a eliminação.

Na otimizada, escreva `d[i]` para a diagonal e `b[i]` para `−F[i]`. A partir da esquerda, o pivô ajustado é `p[i] = d[i] − 2/p[i−1]` e o lado direito normalizado é `w[i] = (b[i] + w[i−1])/p[i]`. A partir da direita, o pivô usa `p[i+1]` e o lado direito é `(b[i] + 2 w[i+1])/p[i]`. As duas cadeias atualizam linhas diferentes. Seus efeitos se encontram na linha central.

Quando `|d[i]| >= 3`, cada pivô lateral tem módulo ao menos 2 por indução. Na equação central, cada lado desconta no máximo 1 do módulo, de forma que sobra um pivô de módulo pelo menos 1. A volta começa com a solução dessa equação e usa os coeficientes `2/p[i]` à esquerda e `1/p[i]` à direita que ficaram guardados. Cada linha exige uma divisão ao todo. A latência de uma cadeia pode ser parcialmente sobreposta ao trabalho da outra no mesmo núcleo.

## Leitura das medidas

Compare **básica revisada × otimizada revisada** para explicar as diferenças entre os algoritmos. Compare **original × revisada da mesma versão** para atribuir o ganho à revisão. O [registro](REGISTRO_OTIMIZACOES.md) contém resultados intermediários e dispersão. A região da Jacobiana escreve apenas a diagonal principal em todas as versões. A região linear inclui o preparo das diagonais de trabalho e, na otimizada, o teste para usar Thomas.
