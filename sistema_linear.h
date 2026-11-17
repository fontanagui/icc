#ifndef SISTEMA_LINEAR_H
#define SISTEMA_LINEAR_H

#include <stdbool.h>
#include "jacobiana.h"

/* Resolve J*passo = rhs. A resposta fica no próprio vetor rhs. */
bool resolve_sistema(size_t n, Tridiagonal j, double *rhs);

#endif
