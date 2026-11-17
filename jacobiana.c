#include "jacobiana.h"

#include "broyden.h"

/* Materializa os coeficientes que o eliminador com pivotamento altera. */
void preenche_laterais(size_t n, Tridiagonal j)
{
    double *restrict inferior = j.inferior;
    double *restrict superior = j.superior;

    for (size_t i = 0; i + 1 < n; ++i) {
        inferior[i] = -1.0;
        superior[i] = -2.0;
    }
}

/* Calcula as derivadas analíticas. A referência materializa toda a banda. */
void constroi_jacobiana(size_t n, const double *restrict x, Tridiagonal j)
{
    double *restrict diagonal = j.diagonal;
#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    x = __builtin_assume_aligned(x, BROYDEN_ALINHAMENTO);
    diagonal = __builtin_assume_aligned(diagonal, BROYDEN_ALINHAMENTO);
#endif

    for (size_t i = 0; i < n; ++i) {
        diagonal[i] = 3.0 - 4.0 * x[i];
    }

#ifdef BROYDEN_BASELINE
    preenche_laterais(n, j);
#endif
}
