#ifndef NEWTON_H
#define NEWTON_H

#include <stddef.h>

#include "utils.h"

typedef struct {
    size_t n;
    double x0;
    double epsilon;
    size_t max;
} Problema;

typedef struct {
    size_t passos;
    rtime_t total;
    rtime_t jacobianas;
    rtime_t sistemas;
} Medidas;

/* Imprime a trajetória e as medidas. Retorna NULL ou a mensagem de falha. */
const char *executa_newton(const Problema *problema);

#endif
