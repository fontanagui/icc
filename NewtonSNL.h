#ifndef NEWTON_SNL_H
#define NEWTON_SNL_H

#include <stddef.h>
#include "utils.h"

/* Tempo de Newton, das Jacobianas e dos sistemas lineares, em ms. */
typedef struct {
    size_t passos;
    rtime_t total;
    rtime_t jacobiana;
    rtime_t gauss;
} Tempos_t;

/* Resolve o problema e imprime os resultados. Retorna 0 se não houve erro. */
int newton( size_t n, double x0, double epsilon, size_t MAXIT );

#endif
