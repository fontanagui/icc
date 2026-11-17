#ifndef JACOBIANA_H
#define JACOBIANA_H

#include <stddef.h>

typedef struct {
    double *diagonal;
    double *inferior; /* inferior[i] representa J[i+1,i]. */
    double *superior; /* superior[i] representa J[i,i+1]. */
} Tridiagonal;

void constroi_jacobiana(size_t n, const double *x, Tridiagonal j);
void preenche_laterais(size_t n, Tridiagonal j);

#endif
