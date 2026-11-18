# Guia do código

1. `main` chama `leEntrada` para ler `N`, o chute inicial comum, a tolerância e o máximo de iterações. Em seguida, chama `newton`.
2. `newton` reserva cinco vetores em um bloco, preenche `X` com o chute inicial e imprime esse vetor. As regiões de medição começam dentro do laço de Newton.
3. `calculaPasso` chama `calcMenosF`. Se a tolerância for positiva, calcula a norma infinito de `−F(X)` e verifica a parada. O sinal é indiferente para a norma.
4. `calcJ` preenche a diagonal principal com `3 − 4 X[i]`. Os coeficientes fixos ao redor dela são `−1` abaixo e `−2` acima.
5. `eliminacaoGauss` sobrescreve o lado direito com o passo `S`. Na otimizada, quando todos os elementos da diagonal são finitos e têm módulo pelo menos 3, `gaussRapido` elimina em duas direções até uma linha central. Nos outros casos e na versão básica, `triang` escolhe entre o pivô da linha e o elemento `−1` abaixo. `retrossubs` recupera a solução de trás para frente. Uma troca pode criar uma segunda superdiagonal.
6. `atualizaX` escreve `X + S` no buffer que guardava `S` e verifica a finitude. `newton` troca os ponteiros dos vetores, imprime o iterado e volta ao laço. O rodapé informa os tempos acumulados sem a impressão dos vetores.

A otimizada mantém as bordas de `calcMenosF` fora do laço. Para `N=1`, o polinômio tem somente o termo local e a constante final. A construção de `J` e a atualização usam `restrict` porque operam em dois vetores disjuntos. Os vetores da versão otimizada começam em endereços alinhados a 64 bytes, inclusive depois da troca de ponteiros. `calcJ` e `atualizaX` recebem esses endereços. Os laços independentes podem ser vetorizados. Cada cadeia de Gauss depende do pivô anterior e é escalar.

O [roteiro técnico](DEFESA_TECNICA.md) mostra como deduzir as recorrências e os limites dos pivôs.
