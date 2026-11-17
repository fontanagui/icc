# Baseline densa

Versão de comparação com Jacobiana cheia e Gauss genérico com pivoteamento
parcial. Construção da Jacobiana O(N²), resolução O(N³), matriz O(N²).

```sh
make
printf '32 -1 0 25\n' | ./broyden
make test
make clean
```

Os testes desta pasta limitam os casos grandes a N=256. Na raiz, `make test`
também compara as duas versões até N=512. A compilação usa as mesmas opções
da versão otimizada; `make LIKWID=1` ativa os marcadores.

As instruções completas, parâmetros, comparação e medições estão no
[README principal](../README.md). Para N=20000, somente a matriz ocupa 3,2 GB;
o custo cúbico pode tornar os testes completos muito demorados.
