#ifndef SIST_NAO_LINEAR_H
#define SIST_NAO_LINEAR_H

#include <stddef.h>

#define ALINHAMENTO 64

/* Calcula o lado direito -F(X) do passo de Newton. */
void calcMenosF( double *restrict b, const double *restrict X, size_t n );

/* Calcula apenas a diagonal principal da matriz Jacobiana. */
void calcJ( double *restrict diagonal, const double *restrict X, size_t n );

/* Calcula a norma infinito de um vetor. */
double norma( const double *V, size_t n );

/* Soma a correção ao vetor X. O resultado fica no próprio S. */
int atualizaX( double *restrict S, const double *restrict X, size_t n );

#endif
