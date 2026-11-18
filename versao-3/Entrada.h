#ifndef ENTRADA_H
#define ENTRADA_H

#include <stddef.h>
#include "NewtonSNL.h"

/* Lê N, x0, epsilon e MAXIT, na ordem solicitada. */
int leEntrada( size_t *n, double *x0, double *epsilon, size_t *MAXIT );

/* Imprime o vetor X, separando as iterações com #. */
void imprimeX( const double *X, size_t n, int primeiro );

/* Imprime os três tempos de execução. */
void imprimeTempos( const Tempos_t *T );

#endif
