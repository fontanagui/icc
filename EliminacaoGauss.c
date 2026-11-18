#include <float.h>
#include <math.h>
#include "EliminacaoGauss.h"

/* Preenche as duas diagonais laterais da Jacobiana. */
void preencheDiagonais( SistLinear_t *SL ) {
    size_t i;

    for ( i = 0; i + 1 < SL->n; i++ ) {
        SL->inferior[i] = -1.0;
        SL->superior[i] = -2.0;
    }
}

/* Faz a triangularização com pivoteamento entre linhas vizinhas. */
static int triang( SistLinear_t *SL ) {
    size_t i;
    double *a = SL->principal;
    double *b = SL->b;
    double *inferior = SL->inferior;
    double *superior = SL->superior;

    for ( i = 0; i + 1 < SL->n; i++ ) {
        double pivo = a[i];
        double abaixo = inferior[i];

        if ( !isfinite( pivo ) )
            return 0;

        if ( fabs( pivo ) >= fabs( abaixo ) ) {
            double m;

            if ( pivo == 0.0 )
                return 0;

            m = abaixo / pivo;
            a[i + 1] -= m * superior[i];
            b[i + 1] -= m * b[i];
            inferior[i] = 0.0;
        } else {
            /* Troca as linhas sem construir uma matriz N x N. */
            double m = pivo / abaixo;
            double proxima = a[i + 1];
            double valor = b[i];

            a[i] = abaixo;
            a[i + 1] = superior[i] - m * proxima;
            superior[i] = proxima;
            b[i] = b[i + 1];
            b[i + 1] = valor - m * b[i];

            /* A troca pode criar uma segunda diagonal superior. */
            if ( i + 2 < SL->n ) {
                inferior[i] = superior[i + 1];
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
    double *inferior = SL->inferior;
    double *superior = SL->superior;

    if ( !isfinite( a[n - 1] ) || fabs( a[n - 1] ) < DBL_MIN )
        return 0;

    b[n - 1] /= a[n - 1];

    if ( n > 1 ) {
        if ( !isfinite( a[n - 2] ) || a[n - 2] == 0.0 )
            return 0;

        b[n - 2] = ( b[n - 2] - superior[n - 2] * b[n - 1] ) / a[n - 2];

        for ( i = n - 2; i > 0; ) {
            i--;

            if ( !isfinite( a[i] ) || a[i] == 0.0 )
                return 0;

            b[i] = ( b[i] - superior[i] * b[i + 1] - inferior[i] * b[i + 2] ) / a[i];
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

/*
 * Versão rápida da eliminação de Gauss para uma matriz tridiagonal.
 * Como os valores laterais são sempre -1 e -2, não precisamos
 * consultá-los nem guardar a matriz completa durante esse cálculo.
 */
static void gaussRapido( SistLinear_t *SL ) {
    size_t i, n = SL->n;
    double *a = SL->principal;
    double *b = SL->b;
    double inverso = 1.0 / a[0];
    double valor = b[0] * inverso;

    a[0] = 2.0 * inverso;
    b[0] = valor;

    /* Triangularização. */
    for ( i = 1; i < n; i++ ) {
        inverso = 1.0 / ( a[i] - 2.0 * inverso );
        valor = ( b[i] + valor ) * inverso;
        a[i] = 2.0 * inverso;
        b[i] = valor;
    }

    /* Retrossubstituição. */
    for ( i = n - 1; i > 0; ) {
        i--;
        valor = b[i] + a[i] * valor;
        b[i] = valor;
    }
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

    /* Quando necessário, usa a eliminação com troca de linhas. */
    preencheDiagonais( SL );

    if ( !triang( SL ) )
        return 0;

    return retrossubs( SL );
}
