# Campanhas e arquivos brutos

`antes/` contém o primeiro ensaio de tempo. `antes-likwid/` registra a compilação instrumentada e a tentativa de campanha com a cópia original. `intermediaria/` compara a fusão do sinal e a eliminação por duas pontas. `ensaios-isolados/` também compara o pivoteamento simplificado e a opção sem alinhamento. `campanha-final/` intercala original e revisão na mesma rodada.

Cada `bruto.tar.gz` guarda os registros por execução, incluindo o aquecimento. Por exemplo, `tar -tzf experimentos/campanha-final/bruto.tar.gz` lista os arquivos. Nos ensaios com LIKWID, `ambiente/` guarda as sondas bloqueadas pela política de acesso. Os CSV de amostras de tempo têm só os valores efetivamente medidos. `campanha.json` registra os parâmetros. A coleta original usou o comando abaixo, trocando `--out` para `experimentos/antes-likwid` na rodada com a ferramenta portátil.

```bash
python3 experimentos/fonte-original/benchmark/coleta.py \
  --out experimentos/antes --cpu 2 --repeticoes 10
```

## Reproduzir os estágios

Os patches são relativos à raiz da cópia preservada em `fonte-original-fbb6084.tar.gz`. O estágio 2 usa o patch 01 seguido do 02. O patch 03 inclui as três alterações e pode ser aplicado diretamente à cópia preservada. O estágio sem alinhamento usa a fonte do estágio 3 e `EXTRA_CPPFLAGS=-DBROYDEN_SEM_ALINHAMENTO`.

```bash
mkdir -p .build/estagios/{01,02,03,04}
for etapa in 01 02 03 04; do
  tar -xzf experimentos/fonte-original-fbb6084.tar.gz -C ".build/estagios/$etapa"
done
for etapa in 01 02; do
  patch -d ".build/estagios/$etapa" -p1 < experimentos/estagios/01-menos-f.patch
done
patch -d .build/estagios/02 -p1 < experimentos/estagios/02-duas-pontas.patch
for etapa in 03 04; do
  patch -d ".build/estagios/$etapa" -p1 < experimentos/estagios/03-acumulado.patch
done
for etapa in 01 02 03; do
  make -C ".build/estagios/$etapa" test LAST_CONSTANT=1
done
make -C .build/estagios/04 test LAST_CONSTANT=1 EXTRA_CPPFLAGS=-DBROYDEN_SEM_ALINHAMENTO

cp benchmark/{coleta,formato}.py .build/estagios/03/benchmark/
python3 .build/estagios/03/benchmark/coleta.py --cpu 2 \
  --out resultados/estagios --repeticoes 10 --so-tempo \
  --original experimentos/fonte-original \
  --binario menos-f=.build/estagios/01/broyden \
  --binario menos-f-baseline=.build/estagios/01/broyden_baseline \
  --binario duas-pontas=.build/estagios/02/broyden \
  --binario duas-pontas-baseline=.build/estagios/02/broyden_baseline \
  --binario sem-alinhamento=.build/estagios/04/broyden
python3 benchmark/analise.py resultados/estagios
```

A sonda local usou o [launcher](verificacao-inicial/launcher-likwid.sh) com LIKWID 5.4.1 e Lua 5.3.6 extraídos em uma pasta de trabalho. Os [checksums](verificacao-inicial/pacotes.txt) identificam os pacotes. Uma instalação regular do LIKWID usa os comandos do [README principal](../README.md).
