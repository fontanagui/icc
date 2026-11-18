#include <math.h>
#include "EliminacaoGauss.h"

int eliminacaoGauss(SistLinear_t *sl, double *b) {
    size_t n = sl->n;
    double *d = sl->d, *du = sl->du, *dl = sl->dl, *du2 = sl->du2;
    for (size_t i = 0; i + 1 < n; ++i) {
        if (!isfinite(d[i]) || !isfinite(b[i])) return -1;
        if (fabs(d[i]) >= fabs(dl[i])) {
            if (d[i] == 0.0) return -1;
            double m = dl[i] / d[i];
            d[i+1] -= m * du[i];
            b[i+1] -= m * b[i];
        } else {
            /* A troca pode criar uma segunda superdiagonal. */
            double m = d[i] / dl[i];
            double proximo = d[i+1];
            d[i] = dl[i];
            d[i+1] = du[i] - m * proximo;
            du[i] = proximo;
            if (i + 2 < n) {
                du2[i] = du[i+1];
                du[i+1] *= -m;
            }
            double anterior = b[i];
            b[i] = b[i+1];
            b[i+1] = anterior - m * b[i+1];
        }
    }
    if (d[n-1] == 0.0 || !isfinite(d[n-1])) return -1;
    b[n-1] /= d[n-1];
    b[n-2] = (b[n-2] - du[n-2]*b[n-1]) / d[n-2];
    for (size_t k = n-2; k > 0; --k) {
        size_t i = k-1;
        b[i] = (b[i] - du[i]*b[i+1] - du2[i]*b[i+2]) / d[i];
    }
    for (size_t i = 0; i < n; ++i)
        if (!isfinite(b[i])) return -1;
    return 0;
}
