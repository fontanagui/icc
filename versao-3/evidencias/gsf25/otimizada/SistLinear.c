#include <stdint.h>
#include <stdlib.h>
#include "SistLinear.h"

SistLinear_t *criaSistLinear(size_t n) {
    if (n < 2 || n > SIZE_MAX / (4 * sizeof(double))) return NULL;
    SistLinear_t *sl = malloc(sizeof(*sl));
    if (!sl) return NULL;
    sl->dados = malloc(4 * n * sizeof(double));
    if (!sl->dados) { free(sl); return NULL; }
    sl->n = n;
    sl->dl = sl->dados;
    sl->d = sl->dl + n;
    sl->du = sl->d + n;
    sl->du2 = sl->du + n;
    return sl;
}

void destroiSistLinear(SistLinear_t *sl) {
    if (sl) { free(sl->dados); free(sl); }
}
