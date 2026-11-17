.DEFAULT_GOAL := all
.PHONY: all clean baseline optimized likwid test sanitize benchmark relatorio-pdf FORCE

CC = gcc
PYTHON ?= python3
PDFLATEX ?= pdflatex
CFLAGS ?= -Wall -Wextra
LAST_CONSTANT ?= 1
FP_CONTRACT ?= fast
LIKWID_CPPFLAGS ?= $(shell pkg-config --cflags likwid 2>/dev/null)
LIKWID_LDFLAGS ?=
LIKWID_LDLIBS ?= -llikwid -pthread
EXTRA_CPPFLAGS ?=

PERF_FLAGS = -std=c11 -O3 -march=native -mavx -fopt-info-vec -ffp-contract=$(FP_CONTRACT)
BASE_CPPFLAGS = $(CPPFLAGS) $(EXTRA_CPPFLAGS) -D_GNU_SOURCE -DBROYDEN_LAST_CONSTANT=$(LAST_CONSTANT)
SAN_FLAGS = -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie
SAN_LDFLAGS = -fsanitize=address,undefined -no-pie
ASAN_OPTIONS ?= detect_leaks=1

SOURCES = main.c io.c newton.c broyden.c jacobiana.c sistema_linear.c utils.c

all optimized: broyden
baseline: broyden_baseline
likwid: broyden_likwid broyden_baseline_likwid

# Cada configuração tem seus próprios objetos e registra alterações de flags.
define BUILD
OBJS_$(1) := $$(SOURCES:%.c=.build/$(1)/%.o)

$(1): $$(OBJS_$(1))
	$$(CC) $$(LDFLAGS) $(4) $$^ $$(LDLIBS) -lm $(5) -o $$@

.build/$(1)/flags: FORCE
	@mkdir -p $$(@D)
	@printf '%s\n' '$$(CC) $$(BASE_CPPFLAGS) $(2) $$(CFLAGS) $$(PERF_FLAGS) $(3) $$(LDFLAGS) $(4) $$(LDLIBS) $(5)' > $$@.tmp
	@cmp -s $$@.tmp $$@ && rm -f $$@.tmp || mv -f $$@.tmp $$@

.build/$(1)/%.o: %.c .build/$(1)/flags Makefile
	$$(CC) $$(BASE_CPPFLAGS) $(2) $$(CFLAGS) $$(PERF_FLAGS) $(3) -MMD -MP -c $$< -o $$@

-include $$(OBJS_$(1):.o=.d)
endef

$(eval $(call BUILD,broyden))
$(eval $(call BUILD,broyden_baseline,-DBROYDEN_BASELINE))
$(eval $(call BUILD,broyden_likwid,-DLIKWID_PERFMON $(LIKWID_CPPFLAGS),,$(LIKWID_LDFLAGS),$(LIKWID_LDLIBS)))
$(eval $(call BUILD,broyden_baseline_likwid,-DBROYDEN_BASELINE -DLIKWID_PERFMON $(LIKWID_CPPFLAGS),,$(LIKWID_LDFLAGS),$(LIKWID_LDLIBS)))
$(eval $(call BUILD,broyden_san,,$(SAN_FLAGS),$(SAN_LDFLAGS)))
$(eval $(call BUILD,broyden_baseline_san,-DBROYDEN_BASELINE,$(SAN_FLAGS),$(SAN_LDFLAGS)))

.build/test_kernels: tests/test_kernels.c $(filter-out .build/broyden/main.o,$(OBJS_broyden))
	$(CC) $(BASE_CPPFLAGS) $(CFLAGS) $(PERF_FLAGS) -I. $^ -lm -o $@

test: broyden broyden_baseline .build/test_kernels
	./.build/test_kernels
	$(PYTHON) tests/test_solver.py $(LAST_CONSTANT) ./broyden ./broyden_baseline
	$(PYTHON) tests/test_likwid_parser.py

sanitize: broyden_san broyden_baseline_san
	ASAN_OPTIONS='$(ASAN_OPTIONS)' UBSAN_OPTIONS=halt_on_error=1 $(PYTHON) tests/test_solver.py $(LAST_CONSTANT) ./broyden_san ./broyden_baseline_san --sanitizers

benchmark:
	$(PYTHON) benchmark/coleta.py --last-constant $(LAST_CONSTANT) $(BENCH_ARGS)

relatorio-pdf:
	mkdir -p .build/relatorio
	$(PDFLATEX) -halt-on-error -interaction=nonstopmode -output-directory=.build/relatorio -jobname=relatorio-broyden-ci1164 relatorio/artigo.tex
	$(PDFLATEX) -halt-on-error -interaction=nonstopmode -output-directory=.build/relatorio -jobname=relatorio-broyden-ci1164 relatorio/artigo.tex
	cp .build/relatorio/relatorio-broyden-ci1164.pdf relatorio-broyden-ci1164.pdf

clean:
	rm -rf .build benchmark/__pycache__ tests/__pycache__
	rm -f broyden broyden_baseline broyden_likwid broyden_baseline_likwid broyden_san broyden_baseline_san
