#include <math.h>
#include "SistNaoLinear.h"

#ifndef BROYDEN_LAST_CONSTANT
#define BROYDEN_LAST_CONSTANT 1
#endif

/* Calcula -F(X) para o sistema linear de Newton. */
void calcMenosF( double *restrict b, const double *restrict X, size_t n ) {
    size_t i;

#ifdef BROYDEN_BASELINE
    /* Versão básica: confere as bordas em todas as posições. */
    for ( i = 0; i < n; i++ ) {
        double esquerda = i == 0 ? 0.0 : X[i - 1];
        double direita = i == n - 1 ? 0.0 : X[i + 1];
        double constante = i == n - 1 ? BROYDEN_LAST_CONSTANT : 1.0;

        b[i] = -( ( 3.0 - 2.0 * X[i] ) * X[i] - esquerda - 2.0 * direita + constante );
    }
#else
    /* Versão otimizada: separa as bordas para não fazer if no laço. */
    if ( n == 1 ) {
        b[0] = -( ( 3.0 - 2.0 * X[0] ) * X[0] + BROYDEN_LAST_CONSTANT );

        return;
    }

    b[0] = -( ( 3.0 - 2.0 * X[0] ) * X[0] - 2.0 * X[1] + 1.0 );

    for ( i = 1; i < n - 1; i++ )
        b[i] = -( ( 3.0 - 2.0 * X[i] ) * X[i] - X[i - 1] - 2.0 * X[i + 1] + 1.0 );

    b[n - 1] = -( ( 3.0 - 2.0 * X[n - 1] ) * X[n - 1] - X[n - 2] + BROYDEN_LAST_CONSTANT );
#endif
}

/* A Jacobiana só tem três diagonais. As laterais são -1 e -2. */
void calcJ( double *restrict diagonal, const double *restrict X, size_t n ) {
    size_t i;

#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    X = __builtin_assume_aligned( X, ALINHAMENTO );
    diagonal = __builtin_assume_aligned( diagonal, ALINHAMENTO );
#endif

    for ( i = 0; i < n; i++ )
        diagonal[i] = 3.0 - 4.0 * X[i];
}

/* Retorna o maior valor absoluto de um vetor. */
double norma( const double *V, size_t n ) {
    size_t i;
    double maior = 0.0;

    for ( i = 0; i < n; i++ ) {
        double valor = fabs( V[i] );

        if ( !isfinite( valor ) )
            return INFINITY;

        if ( valor > maior )
            maior = valor;
    }

    return maior;
}

/* Atualiza X somando o passo S, sem precisar alocar outro vetor. */
int atualizaX( double *restrict S, const double *restrict X, size_t n ) {
    size_t i;
    unsigned int valido = 1;

#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    X = __builtin_assume_aligned( X, ALINHAMENTO );
    S = __builtin_assume_aligned( S, ALINHAMENTO );
#endif

    for ( i = 0; i < n; i++ ) {
        S[i] += X[i];
        valido &= isfinite( S[i] ) != 0;
    }

    return valido != 0;
}
