#include <float.h>
#include <math.h>
#include "sistema_linear.h"

/*
 * Resolve um sistema tridiagonal por eliminação de Gauss.
 * Quando o elemento de baixo é maior, troca as duas linhas.
 * Essa alternativa é usada quando Thomas pode não ser seguro.
 */
static bool gauss_tridiagonal(size_t n, Tridiagonal j, double *restrict rhs)
{
    double *restrict diagonal = j.diagonal;
    double *restrict inferior = j.inferior;
    double *restrict superior = j.superior;

    /* Eliminação: deixa apenas a parte superior da matriz. */
    for (size_t i = 0; i + 1 < n; i++) {
        double pivo = diagonal[i];
        double baixo = inferior[i];

        if (!isfinite(pivo))
            return false;

        if (fabs(pivo) >= fabs(baixo)) {
            if (pivo == 0.0)
                return false;

            double fator = baixo / pivo;
            diagonal[i + 1] -= fator * superior[i];
            rhs[i + 1] -= fator * rhs[i];
            inferior[i] = 0.0;
        } else {
            /* Troca duas linhas sem montar uma matriz completa. */
            double fator = pivo / baixo;
            double proximaDiagonal = diagonal[i + 1];
            double valor = rhs[i];

            diagonal[i] = baixo;
            diagonal[i + 1] = superior[i] - fator * proximaDiagonal;
            superior[i] = proximaDiagonal;
            rhs[i] = rhs[i + 1];
            rhs[i + 1] = valor - fator * rhs[i];

            /* A troca pode criar uma segunda diagonal acima da principal. */
            if (i + 2 < n) {
                inferior[i] = superior[i + 1];
                superior[i + 1] = -fator * superior[i + 1];
            }
        }
    }

    /* Substituição de baixo para cima. */
    if (!isfinite(diagonal[n - 1]) || fabs(diagonal[n - 1]) < DBL_MIN)
        return false;

    rhs[n - 1] /= diagonal[n - 1];

    if (n > 1) {
        rhs[n - 2] = (rhs[n - 2] - superior[n - 2] * rhs[n - 1]) / diagonal[n - 2];

        for (size_t i = n - 2; i > 0;) {
            i--;
            double resto = rhs[i] - superior[i] * rhs[i + 1];
            rhs[i] = (resto - inferior[i] * rhs[i + 2]) / diagonal[i];
        }
    }

    return true;
}

#ifndef BROYDEN_BASELINE
/*
 * Thomas é seguro quando |diagonal[i]| >= 3 em todas as linhas.
 * Nesse caso, os pivôs eliminados ficam longe de zero.
 */
static bool pode_usar_thomas(size_t n, const double *diagonal)
{
    unsigned int seguro = 1;

    for (size_t i = 0; i < n; i++) {
        double valor = fabs(diagonal[i]);
        seguro &= (valor >= 3.0) & (valor <= DBL_MAX);
    }

    return seguro != 0;
}

/*
 * Thomas adaptado ao Broyden: abaixo da diagonal sempre temos -1,
 * e acima sempre -2. Assim não precisamos guardar nem percorrer
 * essas duas diagonais no caso normal.
 */
static void thomas_broyden(size_t n, double *restrict diagonal, double *restrict rhs)
{
    double inverso = 1.0 / diagonal[0];
    double valor = rhs[0] * inverso;

    diagonal[0] = 2.0 * inverso;
    rhs[0] = valor;

    /* Avança pelas linhas, eliminando as variáveis anteriores. */
    for (size_t i = 1; i < n; i++) {
        inverso = 1.0 / (diagonal[i] - 2.0 * inverso);
        valor = (rhs[i] + valor) * inverso;
        diagonal[i] = 2.0 * inverso;
        rhs[i] = valor;
    }

    /* Volta do fim para o começo, encontrando todas as correções. */
    double proximo = valor;
    for (size_t i = n - 1; i > 0;) {
        i--;
        proximo = rhs[i] + diagonal[i] * proximo;
        rhs[i] = proximo;
    }
}
#endif

/* Escolhe o método mais rápido que seja seguro para a Jacobiana atual. */
bool resolve_sistema(size_t n, Tridiagonal j, double *rhs)
{
#ifndef BROYDEN_BASELINE
    if (pode_usar_thomas(n, j.diagonal)) {
        thomas_broyden(n, j.diagonal, rhs);
        return true;
    }

    /* Quando Thomas não serve, precisamos preencher as laterais. */
    preenche_laterais(n, j);
#endif

    return gauss_tridiagonal(n, j, rhs);
}
