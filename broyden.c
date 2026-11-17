#include <math.h>
#include "broyden.h"

/* Calcula (3 - 2*x)*x, que aparece em todas as equações. */
static inline double termo_broyden(double x)
{
    return (3.0 - 2.0 * x) * x;
}

/* Calcula -F(x). O vetor rhs será usado para o passo de Newton. */
void broyden_rhs(size_t n, const double *restrict x, double *restrict rhs)
{
#ifdef BROYDEN_BASELINE
    /* Versão simples: trata as bordas dentro do laço. */
    for (size_t i = 0; i < n; i++) {
        double esquerda = (i == 0) ? 0.0 : x[i - 1];
        double direita = (i == n - 1) ? 0.0 : x[i + 1];
        double constante = (i == n - 1) ? BROYDEN_LAST_CONSTANT : 1.0;

        rhs[i] = -(termo_broyden(x[i]) - esquerda - 2.0 * direita + constante);
    }
#else
    /* Separar as bordas evita vários ifs dentro do laço maior. */
    if (n == 1) {
        rhs[0] = -(termo_broyden(x[0]) + BROYDEN_LAST_CONSTANT);
        return;
    }

    rhs[0] = -(termo_broyden(x[0]) - 2.0 * x[1] + 1.0);

    for (size_t i = 1; i < n - 1; i++)
        rhs[i] = -(termo_broyden(x[i]) - x[i - 1] - 2.0 * x[i + 1] + 1.0);

    rhs[n - 1] = -(termo_broyden(x[n - 1]) - x[n - 2] + BROYDEN_LAST_CONSTANT);
#endif
}

/* Procura o maior valor absoluto do vetor (norma infinito). */
double norma_inf(size_t n, const double *vetor)
{
    double maior = 0.0;

    for (size_t i = 0; i < n; i++) {
        double valor = fabs(vetor[i]);

        if (!isfinite(valor))
            return INFINITY;
        if (valor > maior)
            maior = valor;
    }

    return maior;
}

/* Soma o passo calculado ao x atual, sem usar outro vetor. */
bool aplica_passo(size_t n, const double *restrict x, double *restrict passo)
{
#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    /* Os vetores começam em endereços alinhados: ajuda o compilador a usar AVX. */
    x = __builtin_assume_aligned(x, BROYDEN_ALINHAMENTO);
    passo = __builtin_assume_aligned(passo, BROYDEN_ALINHAMENTO);
#endif
    unsigned int tudoFinito = 1;

    for (size_t i = 0; i < n; i++) {
        double novoX = x[i] + passo[i];
        passo[i] = novoX;
        tudoFinito &= (isfinite(novoX) != 0);
    }

    return tudoFinito != 0;
}
