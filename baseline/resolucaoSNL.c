/* Entrada: N x0 epsilon MAX. N>=2: a fórmula fornecida usa x2 e x(n-1). */
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "NewtonSNL.h"
#include "marcadores.h"

int main(void) {
    long long n, maxit;
    double x0, epsilon;
    if (scanf("%lld %lf %lf %lld", &n, &x0, &epsilon, &maxit) != 4 ||
        n < 2 || n > INT_MAX || maxit < 0 || maxit > INT_MAX ||
        !isfinite(x0) || !isfinite(epsilon) || epsilon < 0.0) {
        fputs("Entrada invalida: informe N>=2, x0 finito, epsilon>=0 e MAX>=0.\n", stderr);
        return 1;
    }
    LIKWID_MARKER_INIT;
    LIKWID_MARKER_THREADINIT;
    ResultadoNewton r;
    int status = newton((size_t)n, x0, epsilon, (size_t)maxit, &r);
    if (status == 0) imprimeResultado(&r);
    else fputs("Falha: memoria insuficiente, Jacobiana singular ou valor nao finito.\n", stderr);
    liberaResultado(&r);
    LIKWID_MARKER_CLOSE;
    return status ? 1 : 0;
}
