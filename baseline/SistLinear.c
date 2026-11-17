#include <stdint.h>
#include <stdlib.h>
#include "SistLinear.h"

SistLinear_t *criaSistLinear(size_t n) {
    if (n < 2 || n > SIZE_MAX / sizeof(double *) ||
        n > SIZE_MAX / sizeof(double) / n) return NULL;
    SistLinear_t *sl = malloc(sizeof(*sl));
    if (!sl) return NULL;
    sl->n = n;
    sl->dados = malloc(n*n*sizeof(double));
    sl->a = malloc(n*sizeof(double *));
    if (!sl->dados || !sl->a) {
        destroiSistLinear(sl);
        return NULL;
    }
    for (size_t i = 0; i < n; ++i) sl->a[i] = sl->dados + i*n;
    return sl;
}

void destroiSistLinear(SistLinear_t *sl) {
    if (sl) { free(sl->a); free(sl->dados); free(sl); }
}
