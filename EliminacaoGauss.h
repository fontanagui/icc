#ifndef ELIMINACAO_GAUSS_H
#define ELIMINACAO_GAUSS_H

#include "SistLinear.h"

/* Resolve o sistema, deixando a solução no próprio vetor b. */
int eliminacaoGauss( SistLinear_t *SL );

/* Preenche as diagonais constantes: -1 abaixo e -2 acima. */
void preencheDiagonais( SistLinear_t *SL );

#endif
