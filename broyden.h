#ifndef BROYDEN_H
#define BROYDEN_H

#include <stdbool.h>
#include <stddef.h>

#ifndef BROYDEN_LAST_CONSTANT
#define BROYDEN_LAST_CONSTANT 1
#endif

#if BROYDEN_LAST_CONSTANT != 0 && BROYDEN_LAST_CONSTANT != 1
#error BROYDEN_LAST_CONSTANT deve ser 0 ou 1
#endif

enum { BROYDEN_ALINHAMENTO = 64 };

/* x e rhs referenciam regiões disjuntas de n doubles. */
void broyden_rhs(size_t n, const double *x, double *rhs);
double norma_inf(size_t n, const double *v);
bool aplica_passo(size_t n, const double *x, double *passo);

#endif
