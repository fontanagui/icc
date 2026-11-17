#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "broyden.h"
#include "jacobiana.h"
#include "sistema_linear.h"

enum { N = 5 };

/* Compara as derivadas do kernel com diferenças centrais em cada coluna. */
static bool testa_jacobiana(void)
{
    _Alignas(64) double x[N] = {-0.7, 0.3, -1.1, 0.6, -0.2};
    _Alignas(64) double principal[N], superior[N], inferior[N];
    double mais[N], menos[N];
    const Tridiagonal j = {principal, inferior, superior};
    const double h = 1e-5;

    constroi_jacobiana(N, x, j);
    preenche_laterais(N, j);
    for (size_t coluna = 0; coluna < N; ++coluna) {
        const double original = x[coluna];
        x[coluna] = original + h;
        broyden_rhs(N, x, mais);
        x[coluna] = original - h;
        broyden_rhs(N, x, menos);
        x[coluna] = original;

        for (size_t linha = 0; linha < N; ++linha) {
            double esperado = 0.0;
            if (linha == coluna) {
                esperado = principal[linha];
            } else if (linha + 1 == coluna) {
                esperado = superior[linha];
            } else if (coluna + 1 == linha) {
                esperado = inferior[coluna];
            }
            const double derivada = (menos[linha] - mais[linha]) / (2 * h);
            if (fabs(derivada - esperado) > 1e-9) {
                return false;
            }
        }
    }
    return true;
}

/* Cria um sistema com solução conhecida e testa a eliminação e a retrocessão. */
static bool testa_sistema(const double original[N])
{
    const double esperado[N] = {2.0, -1.0, 0.5, -3.0, 4.0};
    double principal[N], superior[N], inferior[N], rhs[N];
    const Tridiagonal j = {principal, inferior, superior};

    preenche_laterais(N, j);
    for (size_t i = 0; i < N; ++i) {
        principal[i] = original[i];
        rhs[i] = original[i] * esperado[i];
        if (i > 0) {
            rhs[i] -= esperado[i - 1];
        }
        if (i + 1 < N) {
            rhs[i] -= 2.0 * esperado[i + 1];
        }
    }
    if (!resolve_sistema(N, j, rhs)) {
        return false;
    }
    for (size_t i = 0; i < N; ++i) {
        if (!isfinite(rhs[i]) || fabs(rhs[i] - esperado[i]) > 1e-12) {
            return false;
        }
    }
    return true;
}

/* Exercita pivôs negativos dominantes e trocas com preenchimento de banda. */
int main(void)
{
    const double dominante[N] = {-6.0, 5.0, -9.0, 3.0, 4.0};
    const double com_trocas[N] = {0.0, 2.0, 0.0, 2.0, 3.0};
    const double quase_zero[N] = {1e-16, 2.0, 1e-16, -2.0, 3.0};

    if (!testa_jacobiana() || !testa_sistema(dominante)
        || !testa_sistema(com_trocas) || !testa_sistema(quase_zero)) {
        fputs("Falha nos testes dos kernels.\n", stderr);
        return EXIT_FAILURE;
    }

    puts("Kernels: diferenças centrais e sistemas com solução conhecida verificados.");
    return EXIT_SUCCESS;
}
