#ifndef SIST_LINEAR_H
#define SIST_LINEAR_H

#include <stddef.h>

typedef struct {
    size_t n;
    double *b;
#ifdef BROYDEN_DENSA
    double *dados;
    double **linhas;
#else
    /* O pivotamento usa duas superdiagonais. A subdiagonal vale -1. */
    double *principal;
    double *superior;
    double *segunda;
#endif
} SistLinear_t;

#endif
