CC = gcc
CFLAGS = -O3 -march=native -mavx -fopt-info-vec -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
LDLIBS += -lm
LIKWID ?= 0
# Em instalações fora do caminho padrão, forneça LIKWID_INC e LIKWID_LIB.
ifeq ($(LIKWID),1)
CPPFLAGS += -DLIKWID_PERFMON $(if $(LIKWID_INC),-I$(LIKWID_INC))
LDFLAGS += $(if $(LIKWID_LIB),-L$(LIKWID_LIB))
LDLIBS += -llikwid
endif
OBJDIR = build/likwid-$(LIKWID)
SOURCES = resolucaoSNL.c NewtonSNL.c SistNaoLinear.c SistLinear.c EliminacaoGauss.c utils.c
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)
.DEFAULT_GOAL := all
.PHONY: all clean test FORCE
all: broyden
# Re-link permite alternar LIKWID=0/1 sem reutilizar o executável incorreto.
broyden: $(OBJECTS) FORCE
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)
$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
-include $(OBJECTS:.o=.d)
test: broyden
	python3 ../tests/test_broyden.py ./broyden
clean:
	rm -rf build
	rm -f broyden *.o *.d resolucaoSNL *~ *.bak
