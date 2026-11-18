#ifndef SIST_LINEAR_H
#define SIST_LINEAR_H
#include <stddef.h>
/* Quatro diagonais contíguas; du2 guarda o preenchimento do pivoteamento. */
typedef struct {
    size_t n;
    double *dados, *dl, *d, *du, *du2;
} SistLinear_t;
/* Aloca as diagonais uma única vez. Retorna NULL em caso de falha. */
SistLinear_t *criaSistLinear(size_t n);
void destroiSistLinear(SistLinear_t *sl);
#endif
