#include <math.h>
#include "EliminacaoGauss.h"

/* Gauss genérico: busca pivô em toda a coluna e atualiza a submatriz inteira.
 * Não ignora linhas com multiplicador zero: é a implementação densa usual.
 * A troca de ponteiros evita copiar linhas sem alterar a representação cheia. */
int eliminacaoGauss(SistLinear_t *sl, double *b) {
    size_t n = sl->n;
    double **a = sl->a;
    for (size_t k = 0; k < n; ++k) {
        size_t p = k;
        double maior = fabs(a[k][k]);
        if (!isfinite(maior)) return -1;
        for (size_t i = k+1; i < n; ++i) {
            double v = fabs(a[i][k]);
            if (!isfinite(v)) return -1;
            if (v > maior) { maior = v; p = i; }
        }
        if (maior == 0.0) return -1;
        if (p != k) {
            double *linha = a[k]; a[k] = a[p]; a[p] = linha;
            double valor = b[k]; b[k] = b[p]; b[p] = valor;
        }
        for (size_t i = k+1; i < n; ++i) {
            double m = a[i][k] / a[k][k];
            a[i][k] = 0.0;
            for (size_t j = k+1; j < n; ++j)
                a[i][j] -= m * a[k][j];
            b[i] -= m * b[k];
        }
    }
    /* Substituição regressiva densa, também percorre os zeros. */
    for (size_t k = n; k > 0; --k) {
        size_t i = k-1;
        double soma = b[i];
        for (size_t j = i+1; j < n; ++j) soma -= a[i][j]*b[j];
        b[i] = soma / a[i][i];
        if (!isfinite(b[i])) return -1;
    }
    return 0;
}
