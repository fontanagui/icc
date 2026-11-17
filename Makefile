# Compilação do Broyden (versão otimizada e versão básica).
CC = gcc
PYTHON ?= python3
PDFLATEX ?= pdflatex
LAST_CONSTANT ?= 1
FP_CONTRACT ?= fast

# Flags obrigatórias do professor.
FLAGS = -std=c11 -Wall -Wextra -O3 -march=native -mavx -fopt-info-vec -ffp-contract=$(FP_CONTRACT)
PARAMETROS = -D_GNU_SOURCE -DBROYDEN_LAST_CONSTANT=$(LAST_CONSTANT) $(EXTRA_CPPFLAGS)
FLAGS_SAN = -std=c11 -Wall -Wextra -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie

LIKWID_CPPFLAGS ?= $(shell pkg-config --cflags likwid 2>/dev/null)
LIKWID_LDFLAGS ?=
LIKWID_LDLIBS ?= -llikwid -pthread

FONTES = main.c io.c newton.c broyden.c jacobiana.c sistema_linear.c utils.c
OBJ_OTIM = $(FONTES:%.c=.build/otim/%.o)
OBJ_BASE = $(FONTES:%.c=.build/base/%.o)
OBJ_LIKWID = $(FONTES:%.c=.build/likwid/%.o)
OBJ_BASE_LIKWID = $(FONTES:%.c=.build/base_likwid/%.o)
OBJ_SAN = $(FONTES:%.c=.build/san/%.o)
OBJ_BASE_SAN = $(FONTES:%.c=.build/base_san/%.o)

.DEFAULT_GOAL := all
.PHONY: all clean baseline optimized likwid test sanitize benchmark relatorio-pdf

all: broyden
optimized: broyden
baseline: broyden_baseline
likwid: broyden_likwid broyden_baseline_likwid

# Executáveis: cada versão tem seus próprios arquivos .o.
broyden: $(OBJ_OTIM)
	$(CC) $^ -lm -o $@

broyden_baseline: $(OBJ_BASE)
	$(CC) $^ -lm -o $@

broyden_likwid: $(OBJ_LIKWID)
	$(CC) $(LIKWID_LDFLAGS) $^ -lm $(LIKWID_LDLIBS) -o $@

broyden_baseline_likwid: $(OBJ_BASE_LIKWID)
	$(CC) $(LIKWID_LDFLAGS) $^ -lm $(LIKWID_LDLIBS) -o $@

broyden_san: $(OBJ_SAN)
	$(CC) -fsanitize=address,undefined -no-pie $^ -lm -o $@

broyden_baseline_san: $(OBJ_BASE_SAN)
	$(CC) -fsanitize=address,undefined -no-pie $^ -lm -o $@

# Compila o código otimizado.
.build/otim/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(PARAMETROS) $(FLAGS) -MMD -MP -c $< -o $@

# Compila a versão básica para a comparação do professor.
.build/base/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(PARAMETROS) -DBROYDEN_BASELINE $(FLAGS) -MMD -MP -c $< -o $@

# Versões instrumentadas com os marcadores LIKWID.
.build/likwid/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(PARAMETROS) -DLIKWID_PERFMON $(LIKWID_CPPFLAGS) $(FLAGS) -MMD -MP -c $< -o $@

.build/base_likwid/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(PARAMETROS) -DBROYDEN_BASELINE -DLIKWID_PERFMON $(LIKWID_CPPFLAGS) $(FLAGS) -MMD -MP -c $< -o $@

# Sanitizers são usados só nos testes, nunca nas medições de velocidade.
.build/san/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(PARAMETROS) $(FLAGS_SAN) -MMD -MP -c $< -o $@

.build/base_san/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(PARAMETROS) -DBROYDEN_BASELINE $(FLAGS_SAN) -MMD -MP -c $< -o $@

# Atualiza dependências automaticamente se um arquivo .h mudar.
-include $(OBJ_OTIM:.o=.d) $(OBJ_BASE:.o=.d)
-include $(OBJ_LIKWID:.o=.d) $(OBJ_BASE_LIKWID:.o=.d)
-include $(OBJ_SAN:.o=.d) $(OBJ_BASE_SAN:.o=.d)

.build/test_kernels: tests/test_kernels.c $(filter-out .build/otim/main.o,$(OBJ_OTIM))
	$(CC) $(PARAMETROS) $(FLAGS) -I. $^ -lm -o $@

test: broyden broyden_baseline .build/test_kernels
	./.build/test_kernels
	$(PYTHON) tests/test_solver.py $(LAST_CONSTANT) ./broyden ./broyden_baseline
	$(PYTHON) tests/test_likwid_parser.py

sanitize: broyden_san broyden_baseline_san
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 $(PYTHON) tests/test_solver.py $(LAST_CONSTANT) ./broyden_san ./broyden_baseline_san --sanitizers

benchmark:
	$(PYTHON) benchmark/coleta.py --last-constant $(LAST_CONSTANT) $(BENCH_ARGS)

relatorio-pdf:
	mkdir -p .build/relatorio
	$(PDFLATEX) -halt-on-error -interaction=nonstopmode -output-directory=.build/relatorio -jobname=relatorio-broyden-ci1164 relatorio/artigo.tex
	$(PDFLATEX) -halt-on-error -interaction=nonstopmode -output-directory=.build/relatorio -jobname=relatorio-broyden-ci1164 relatorio/artigo.tex
	cp .build/relatorio/relatorio-broyden-ci1164.pdf relatorio-broyden-ci1164.pdf

# Após mudar LAST_CONSTANT, FP_CONTRACT ou EXTRA_CPPFLAGS, rode make clean.
clean:
	rm -rf .build benchmark/__pycache__ tests/__pycache__
	rm -f broyden broyden_baseline broyden_likwid broyden_baseline_likwid broyden_san broyden_baseline_san
