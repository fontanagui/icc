# Versão otimizada

Jacobiana representada por diagonais e Gauss tridiagonal com pivoteamento
parcial. Construção da Jacobiana e resolução O(N), espaço numérico O(N),
além do histórico de saída O((MAX+1)*N).

```sh
make
printf '32 -1 0 25\n' | ./broyden
make test
make clean
```

Os testes desta pasta incluem os 12 tamanhos exigidos até N=20000.
`make LIKWID=1` ativa os marcadores. Veja o
[README principal](../README.md) para instruções completas e comparação.
