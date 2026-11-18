#ifndef DENSA_H
#define DENSA_H

#include "SistLinear.h"

int alocaMatrizDensa( SistLinear_t *SL );
void liberaMatrizDensa( SistLinear_t *SL );
void calcJDensa( SistLinear_t *SL, const double *X );

#endif
