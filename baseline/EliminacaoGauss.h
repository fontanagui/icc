#ifndef ELIM_GAUSS_H
#define ELIM_GAUSS_H
#include "SistLinear.h"
/* Gauss denso com pivoteamento parcial, O(n³).
 * Destrói a matriz e substitui b pela solução. Retorna -1 se singular/não finito. */
int eliminacaoGauss(SistLinear_t *sl, double *b);
#endif
