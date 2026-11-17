#ifndef BROYDEN_H
#define BROYDEN_H

#include <stdbool.h>
#include <stddef.h>

/* O termo constante da última equação depende da versão do enunciado. */
#ifndef BROYDEN_LAST_CONSTANT
#define BROYDEN_LAST_CONSTANT 1
#endif

#if BROYDEN_LAST_CONSTANT != 0 && BROYDEN_LAST_CONSTANT != 1
#error BROYDEN_LAST_CONSTANT deve ser 0 ou 1
#endif

/* Alinhamento dos vetores para facilitar o uso de instruções AVX. */
#define BROYDEN_ALINHAMENTO 64

/* Calcula -F(x), a norma infinito e o novo vetor após o passo. */
void broyden_rhs(size_t n, const double *x, double *rhs);
double norma_inf(size_t n, const double *vetor);
bool aplica_passo(size_t n, const double *x, double *passo);

#endif
