#include "sistema_linear.h"

#include <float.h>
#include <math.h>

/* Elimina a banda com pivotamento parcial entre duas linhas adjacentes. */
static bool gauss_tridiagonal(size_t n, Tridiagonal j, double *restrict rhs)
{
    double *restrict d = j.diagonal;
    double *restrict inferior = j.inferior;
    double *restrict superior = j.superior;

    for (size_t i = 0; i + 1 < n; ++i) {
        const double pivo = d[i];
        const double abaixo = inferior[i];
        if (!isfinite(pivo)) {
            return false;
        }

        if (fabs(pivo) >= fabs(abaixo)) {
            const double fator = abaixo / pivo;
            d[i + 1] -= fator * superior[i];
            rhs[i + 1] -= fator * rhs[i];
            inferior[i] = 0.0;
        } else {
            const double fator = pivo / abaixo;
            const double proxima_diagonal = d[i + 1];
            d[i] = abaixo;
            d[i + 1] = superior[i] - fator * proxima_diagonal;
            superior[i] = proxima_diagonal;
            const double b = rhs[i];
            rhs[i] = rhs[i + 1];
            rhs[i + 1] = b - fator * rhs[i];

            /* A troca gera uma segunda superdiagonal na linha já eliminada. */
            if (i + 2 < n) {
                inferior[i] = superior[i + 1];
                superior[i + 1] = -fator * superior[i + 1];
            }
        }
    }

    if (!isfinite(d[n - 1]) || fabs(d[n - 1]) < DBL_MIN) {
        return false;
    }

    rhs[n - 1] /= d[n - 1];
    if (n > 1) {
        rhs[n - 2] = (rhs[n - 2] - superior[n - 2] * rhs[n - 1]) / d[n - 2];
        for (size_t i = n - 2; i > 0;) {
            --i;
            const double restante = rhs[i] - superior[i] * rhs[i + 1];
            rhs[i] = (restante - inferior[i] * rhs[i + 2]) / d[i];
        }
    }

    return true;
}

#ifndef BROYDEN_BASELINE
/* Certifica Thomas: |d_i| >= 3 implica |p_i| >= 2 por indução. */
static bool admite_thomas(size_t n, const double *diagonal)
{
    unsigned int valida = 1;

    for (size_t i = 0; i < n; ++i) {
        const double abs_d = fabs(diagonal[i]);
        valida &= (abs_d >= 3.0) & (abs_d <= DBL_MAX);
    }

    return valida != 0;
}

/* Normaliza cada linha com um recíproco e reutiliza-o na retrocessão. */
static void thomas_broyden(size_t n, double *restrict d, double *restrict rhs)
{
    double inverso = 1.0 / d[0];
    double reduzido = rhs[0] * inverso;
    d[0] = 2.0 * inverso;
    rhs[0] = reduzido;

    for (size_t i = 1; i < n; ++i) {
        inverso = 1.0 / (d[i] - 2.0 * inverso);
        reduzido = (rhs[i] + reduzido) * inverso;
        d[i] = 2.0 * inverso;
        rhs[i] = reduzido;
    }

    double proximo = reduzido;
    for (size_t i = n - 1; i > 0;) {
        --i;
        proximo = rhs[i] + d[i] * proximo;
        rhs[i] = proximo;
    }
}
#endif

/* Seleciona o caminho certificado ou o pivô parcial na representação em banda. */
bool resolve_sistema(size_t n, Tridiagonal j, double *rhs)
{
#ifndef BROYDEN_BASELINE
    if (admite_thomas(n, j.diagonal)) {
        thomas_broyden(n, j.diagonal, rhs);
        return true;
    }
    preenche_laterais(n, j);
#endif
    return gauss_tridiagonal(n, j, rhs);
}
