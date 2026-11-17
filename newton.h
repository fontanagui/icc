#ifndef NEWTON_H
#define NEWTON_H

#include <stddef.h>
#include "utils.h"

/* Dados lidos da entrada padrão. */
typedef struct {
    size_t n;
    double x0;
    double epsilon;
    size_t max;
} Problema;

/* Tempos acumulados durante o método de Newton, em milissegundos. */
typedef struct {
    size_t passos;
    rtime_t total;
    rtime_t jacobianas;
    rtime_t sistemas;
} Medidas;

/* Devolve NULL quando termina normalmente; caso contrário, uma mensagem de erro. */
const char *executa_newton(const Problema *problema);

#endif
