#ifndef SIST_LINEAR_H
#define SIST_LINEAR_H

#include <stddef.h>

/* Sistema linear J*S = b. Guardamos só três diagonais da Jacobiana. */
typedef struct {
    size_t n;
    double *principal;
    double *inferior;
    double *superior;
    double *b;
} SistLinear_t;

#endif
