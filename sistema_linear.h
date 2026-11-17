#ifndef SISTEMA_LINEAR_H
#define SISTEMA_LINEAR_H

#include <stdbool.h>

#include "jacobiana.h"

/* Resolve J*d = rhs em memória disjunta, substituindo rhs por d. */
bool resolve_sistema(size_t n, Tridiagonal j, double *rhs);

#endif
