#ifndef SIST_NAO_LINEAR_H
#define SIST_NAO_LINEAR_H
#include "SistLinear.h"
/* Calcula F da imagem: a última equação NÃO possui o termo constante +1. */
void calcF(double *restrict f, const double *restrict x, size_t n);
/* Derivadas analíticas: diagonal 3-4*x[i], inferior -1, superior -2. */
void calcJ(SistLinear_t *sl, const double *restrict x);
/* Norma infinito; valores não finitos produzem infinito. */
double norma(const double *v, size_t n);
#endif
