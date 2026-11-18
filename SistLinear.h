#ifndef SIST_LINEAR_H
#define SIST_LINEAR_H

#include <stddef.h>

/* O pivotamento usa duas superdiagonais. A subdiagonal vale -1. */
typedef struct {
    size_t n;
    double *principal;
    double *superior;
    double *segunda;
    double *b;
} SistLinear_t;

#endif
