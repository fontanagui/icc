# Interface principal: mantém ./broyden como executável padrão do trabalho.
.DEFAULT_GOAL := all
.PHONY: all otimizada baseline versoes test clean
all: broyden

broyden: otimizada
	cp otimizada/broyden broyden

otimizada:
	$(MAKE) -C otimizada

baseline:
	$(MAKE) -C baseline

versoes: broyden baseline

test: versoes
	python3 -B tests/test_broyden.py ./broyden
	python3 -B tests/test_broyden.py ./baseline/broyden --rapido
	python3 -B tests/test_comparacao.py

clean:
	$(MAKE) -C otimizada clean
	$(MAKE) -C baseline clean
	rm -f broyden
	rm -rf tests/__pycache__ scripts/__pycache__
