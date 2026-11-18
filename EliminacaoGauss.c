#include <float.h>
#include <math.h>
#include "EliminacaoGauss.h"

/* Faz a triangularização com pivoteamento entre linhas vizinhas. */
static int triang( SistLinear_t *SL ) {
    size_t i;
    double *a = SL->principal;
    double *b = SL->b;
    double *superior = SL->superior;
    double *segunda = SL->segunda;

    for ( i = 0; i + 1 < SL->n; i++ )
        superior[i] = -2.0;

    for ( i = 0; i + 1 < SL->n; i++ ) {
        double pivo = a[i];

        if ( !isfinite( pivo ) )
            return 0;

        if ( fabs( pivo ) >= 1.0 ) {
            double m = -1.0 / pivo;

            a[i + 1] -= m * superior[i];
            b[i + 1] -= m * b[i];
            segunda[i] = 0.0;
        } else {
            /* Troca as linhas sem construir uma matriz N x N. */
            double m = -pivo;
            double proxima = a[i + 1];
            double valor = b[i];

            a[i] = -1.0;
            a[i + 1] = superior[i] - m * proxima;
            superior[i] = proxima;
            b[i] = b[i + 1];
            b[i + 1] = valor - m * b[i];

            /* A troca pode criar uma segunda diagonal superior. */
            if ( i + 2 < SL->n ) {
                segunda[i] = superior[i + 1];
                superior[i + 1] = -m * superior[i + 1];
            }
        }
    }

    return 1;
}

/* Calcula as incógnitas da última linha para a primeira. */
static int retrossubs( SistLinear_t *SL ) {
    size_t i, n = SL->n;
    double *a = SL->principal;
    double *b = SL->b;
    double *superior = SL->superior;
    double *segunda = SL->segunda;

    /* Os pivôs anteriores são finitos e têm módulo maior ou igual a 1. */
    if ( !isfinite( a[n - 1] ) || a[n - 1] == 0.0 )
        return 0;

    b[n - 1] /= a[n - 1];

    if ( n > 1 ) {
        b[n - 2] = ( b[n - 2] - superior[n - 2] * b[n - 1] ) / a[n - 2];

        for ( i = n - 2; i > 0; ) {
            i--;

            b[i] = ( b[i] - superior[i] * b[i + 1] - segunda[i] * b[i + 2] ) / a[i];
        }
    }

    return 1;
}

#ifndef BROYDEN_BASELINE
/* Confere se é seguro usar a eliminação mais rápida. */
static int pivosSeguros( SistLinear_t *SL ) {
    size_t i;
    unsigned int seguro = 1;

    for ( i = 0; i < SL->n; i++ ) {
        double valor = fabs( SL->principal[i] );
        seguro &= ( valor >= 3.0 ) & ( valor <= DBL_MAX );
    }

    return seguro != 0;
}

/* Elimina das duas pontas para intercalar duas cadeias de divisões. */
static void gaussRapido( SistLinear_t *SL ) {
    size_t i, j, n = SL->n, meio = ( n - 1 ) / 2;
    double *a = SL->principal;
    double *b = SL->b;
    double invEsq = 0.0, invDir = 0.0;
    double esq = 0.0, dir = 0.0;

    for ( i = 0, j = n - 1; i < meio; i++, j-- ) {
        invEsq = 1.0 / ( a[i] - 2.0 * invEsq );
        invDir = 1.0 / ( a[j] - 2.0 * invDir );
        esq = ( b[i] + esq ) * invEsq;
        dir = ( b[j] + 2.0 * dir ) * invDir;
        a[i] = 2.0 * invEsq;
        a[j] = invDir;
        b[i] = esq;
        b[j] = dir;
    }

    /* N par deixa uma linha a mais no lado direito. */
    if ( j > meio ) {
        invDir = 1.0 / ( a[j] - 2.0 * invDir );
        dir = ( b[j] + 2.0 * dir ) * invDir;
        a[j] = invDir;
        b[j] = dir;
    }

    /* Resolve a equação central e volta às pontas. */
    b[meio] = ( b[meio] + esq + 2.0 * dir ) / ( a[meio] - 2.0 * invEsq - 2.0 * invDir );
    esq = b[meio];
    dir = esq;

    for ( j = meio; i > 0; ) {
        i--;
        j++;
        esq = b[i] + a[i] * esq;
        dir = b[j] + a[j] * dir;
        b[i] = esq;
        b[j] = dir;
    }

    if ( j + 1 < n )
        b[j + 1] += a[j + 1] * dir;
}
#endif

/* Resolve J*S = b com a versão mais adequada da eliminação de Gauss. */
int eliminacaoGauss( SistLinear_t *SL ) {
#ifndef BROYDEN_BASELINE
    if ( pivosSeguros( SL ) ) {
        gaussRapido( SL );

        return 1;
    }
#endif

    if ( !triang( SL ) )
        return 0;

    return retrossubs( SL );
}
