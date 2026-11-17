#ifndef JACOBIANA_H
#define JACOBIANA_H

#include <stddef.h>

/* Guarda apenas as três diagonais da Jacobiana, sem uma matriz N x N. */
typedef struct {
    double *diagonal;
    double *inferior;  /* posição i representa a linha i+1 */
    double *superior;  /* posição i representa a linha i */
} Tridiagonal;

void constroi_jacobiana(size_t n, const double *x, Tridiagonal j);
void preenche_laterais(size_t n, Tridiagonal j);

#endif
