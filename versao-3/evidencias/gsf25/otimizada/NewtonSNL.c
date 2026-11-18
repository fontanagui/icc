#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NewtonSNL.h"
#include "SistNaoLinear.h"
#include "EliminacaoGauss.h"
#include "marcadores.h"
#include "utils.h"

int newton(size_t n, double x0, double epsilon, size_t maxit, ResultadoNewton *r) {
    *r = (ResultadoNewton){.n = n};
    if (n < 2 || !isfinite(x0) || !isfinite(epsilon) || epsilon < 0.0 ||
        maxit == SIZE_MAX || n > SIZE_MAX / sizeof(double) / (maxit+1)) return -1;
    r->historico = malloc((maxit+1)*n*sizeof(double));
    double *x = malloc(n*sizeof(double)), *s = malloc(n*sizeof(double));
    SistLinear_t *sl = criaSistLinear(n);
    if (!r->historico || !x || !s || !sl) {
        free(x); free(s); destroiSistLinear(sl); liberaResultado(r); return -1;
    }
    for (size_t i = 0; i < n; ++i) x[i] = x0;
    memcpy(r->historico, x, n*sizeof(double));
    LIKWID_MARKER_REGISTER("NEWTON");
    LIKWID_MARKER_REGISTER("JACOBIANA");
    LIKWID_MARKER_REGISTER("SISTEMA_LINEAR");
    int status = 0;
    LIKWID_MARKER_START("NEWTON");
    double inicio = timestamp();
    for (size_t k = 0; k < maxit; ++k) {
        calcF(s, x, n);
        double residuo = norma(s, n);
        if (!isfinite(residuo)) { status = -1; break; }
        if (epsilon > 0.0 && residuo <= epsilon) break;
        LIKWID_MARKER_START("JACOBIANA");
        double t = timestamp();
        calcJ(sl, x);
        r->jacobiana_ms += timestamp() - t;
        LIKWID_MARKER_STOP("JACOBIANA");
        for (size_t i = 0; i < n; ++i) s[i] = -s[i];
        LIKWID_MARKER_START("SISTEMA_LINEAR");
        t = timestamp();
        status = eliminacaoGauss(sl, s);
        r->linear_ms += timestamp() - t;
        LIKWID_MARKER_STOP("SISTEMA_LINEAR");
        if (status) break;
        for (size_t i = 0; i < n; ++i) x[i] += s[i];
        if (!isfinite(norma(x, n))) { status = -1; break; }
        ++r->iteracoes;
        memcpy(r->historico + r->iteracoes*n, x, n*sizeof(double));
        if (epsilon > 0.0 && norma(s, n) <= epsilon) break;
    }
    r->total_ms = timestamp() - inicio;
    LIKWID_MARKER_STOP("NEWTON");
    free(x); free(s); destroiSistLinear(sl);
    return status;
}

void imprimeResultado(const ResultadoNewton *r) {
    for (size_t k = 0; k <= r->iteracoes; ++k) {
        if (k) puts("#");
        for (size_t i = 0; i < r->n; ++i)
            printf("x%zu = %.17g\n", i+1, r->historico[k*r->n+i]);
    }
    printf("###########\n%.9f\n%.9f\n%.9f\n###########\n",
           r->total_ms, r->jacobiana_ms, r->linear_ms);
}

void liberaResultado(ResultadoNewton *r) {
    free(r->historico);
    r->historico = NULL;
}
