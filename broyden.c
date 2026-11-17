#include "broyden.h"

#include <math.h>

/* Avalia o termo que depende apenas da incógnita da própria linha. */
static inline double termo_local(double x)
{
    return (3.0 - 2.0 * x) * x;
}

/* Escreve -F(X). O caminho interno otimizado tem vizinhos sempre válidos. */
void broyden_rhs(size_t n, const double *restrict x, double *restrict rhs)
{
#ifdef BROYDEN_BASELINE
    for (size_t i = 0; i < n; ++i) {
        const double esquerda = i == 0 ? 0.0 : x[i - 1];
        const double direita = i + 1 == n ? 0.0 : x[i + 1];
        const double constante = i + 1 == n ? BROYDEN_LAST_CONSTANT : 1.0;
        rhs[i] = -(termo_local(x[i]) - esquerda - 2.0 * direita + constante);
    }
#else
    if (n == 1) {
        rhs[0] = -(termo_local(x[0]) + BROYDEN_LAST_CONSTANT);
        return;
    }

    rhs[0] = -(termo_local(x[0]) - 2.0 * x[1] + 1.0);
    for (size_t i = 1; i < n - 1; ++i) {
        rhs[i] = -(termo_local(x[i]) - x[i - 1] - 2.0 * x[i + 1] + 1.0);
    }
    rhs[n - 1] = -(termo_local(x[n - 1]) - x[n - 2] + BROYDEN_LAST_CONSTANT);
#endif
}

/* Calcula max_i |v_i| e sinaliza componentes não finitas com infinito. */
double norma_inf(size_t n, const double *v)
{
    double maximo = 0.0;

    for (size_t i = 0; i < n; ++i) {
        const double abs_v = fabs(v[i]);
        if (!isfinite(abs_v)) {
            return INFINITY;
        }
        if (abs_v > maximo) {
            maximo = abs_v;
        }
    }

    return maximo;
}

/* Transforma o buffer de correções no próximo X e testa a finitude. */
bool aplica_passo(size_t n, const double *restrict x, double *restrict passo)
{
#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    x = __builtin_assume_aligned(x, BROYDEN_ALINHAMENTO);
    passo = __builtin_assume_aligned(passo, BROYDEN_ALINHAMENTO);
#endif
    unsigned int finito = 1;

    for (size_t i = 0; i < n; ++i) {
        const double novo = x[i] + passo[i];
        passo[i] = novo;
        finito &= (isfinite(novo) != 0);
    }

    return finito != 0;
}
