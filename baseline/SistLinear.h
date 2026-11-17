#ifndef SIST_LINEAR_H
#define SIST_LINEAR_H
#include <stddef.h>
/* Matriz cheia N x N contígua com ponteiros de linhas para pivoteamento. */
typedef struct {
    size_t n;
    double *dados, **a;
} SistLinear_t;
/* Aloca uma única vez. Retorna NULL em caso de overflow ou falha de alocação. */
SistLinear_t *criaSistLinear(size_t n);
void destroiSistLinear(SistLinear_t *sl);
#endif
