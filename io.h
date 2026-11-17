#ifndef IO_H
#define IO_H

#include <stdbool.h>

#include "newton.h"

bool le_problema(Problema *problema);
void imprime_iteracao(size_t n, const double *x, bool primeira);
void imprime_medidas(const Medidas *medidas);

#endif
