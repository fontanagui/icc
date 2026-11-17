#include <math.h>
#include "SistNaoLinear.h"

void calcF(double *restrict f, const double *restrict x, size_t n) {
    f[0] = (3.0 - 2.0*x[0])*x[0] - 2.0*x[1] + 1.0;
    for (size_t i = 1; i + 1 < n; ++i)
        f[i] = (3.0 - 2.0*x[i])*x[i] - x[i-1] - 2.0*x[i+1] + 1.0;
    f[n-1] = (3.0 - 2.0*x[n-1])*x[n-1] - x[n-2];
}

void calcJ(SistLinear_t *sl, const double *restrict x) {
    double *restrict d = sl->d;
    double *restrict dl = sl->dl;
    double *restrict du = sl->du;
    double *restrict du2 = sl->du2;
    for (size_t i = 0; i < sl->n; ++i) {
        d[i] = 3.0 - 4.0*x[i];
        dl[i] = -1.0;
        du[i] = -2.0;
        du2[i] = 0.0;
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
