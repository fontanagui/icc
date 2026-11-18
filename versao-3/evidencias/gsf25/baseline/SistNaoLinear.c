#include <math.h>
#include "SistNaoLinear.h"

void calcF(double *restrict f, const double *restrict x, size_t n) {
    f[0] = (3.0 - 2.0*x[0])*x[0] - 2.0*x[1] + 1.0;
    for (size_t i = 1; i + 1 < n; ++i)
        f[i] = (3.0 - 2.0*x[i])*x[i] - x[i-1] - 2.0*x[i+1] + 1.0;
    f[n-1] = (3.0 - 2.0*x[n-1])*x[n-1] - x[n-2];
}

void calcJ(SistLinear_t *sl, const double *restrict x) {
    /* Representação densa: visita inclusive elementos estruturalmente nulos. */
    for (size_t i = 0; i < sl->n; ++i) {
        for (size_t j = 0; j < sl->n; ++j) {
            if (i == j) sl->a[i][j] = 3.0 - 4.0*x[i];
            else if (i == j+1) sl->a[i][j] = -1.0;
            else if (j == i+1) sl->a[i][j] = -2.0;
            else sl->a[i][j] = 0.0;
        }
    }
}

double norma(const double *v, size_t n) {
    double m = 0.0;
    for (size_t i = 0; i < n; ++i) {
        if (!isfinite(v[i])) return INFINITY;
        if (fabs(v[i]) > m) m = fabs(v[i]);
    }
    return m;
}
