#ifndef ELIM_GAUSS_H
#define ELIM_GAUSS_H
#include "SistLinear.h"
/* Gauss tridiagonal com pivoteamento parcial entre linhas adjacentes, O(n).
 * Destrói diagonais e substitui b pela solução. Retorna -1 se singular/não finito. */
int eliminacaoGauss(SistLinear_t *sl, double *b);
#endif
