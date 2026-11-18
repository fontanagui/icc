# Status dos dados e relatório

Os dados experimentais, gráficos e PDF conservados neste ZIP foram feitos com a versão anterior do projeto. Servem como histórico e referência metodológica; **não** comprovam o desempenho desta reescrita.

Os testes de correção, sanitizers e um ensaio rápido de desempenho foram feitos nesta versão. Antes da entrega, repita a campanha de LIKWID na máquina apropriada, gere os 12 gráficos pedidos e atualize o artigo científico com os números efetivamente medidos.

Confira também a imagem oficial do enunciado para a última equação: a função tridiagonal clássica termina com `+1`, enquanto o script `broyden.sh` fornecido não coloca essa constante. O `Makefile` usa `LAST_CONSTANT=1` por padrão; pode receber `LAST_CONSTANT=0` caso essa seja a definição do professor.
