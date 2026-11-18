#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "Densa.h"
#include "EliminacaoGauss.h"

int alocaMatrizDensa( SistLinear_t *SL ) {
    size_t n = SL->n;

    if ( n > SIZE_MAX / sizeof( *SL->linhas ) || n > SIZE_MAX / sizeof( *SL->dados ) / n )
        return 0;

    SL->linhas = malloc( n * sizeof( *SL->linhas ) );
    SL->dados = malloc( n * n * sizeof( *SL->dados ) );

    if ( SL->linhas == NULL || SL->dados == NULL ) {
        liberaMatrizDensa( SL );
        return 0;
    }

    for ( size_t i = 0; i < n; i++ )
        SL->linhas[i] = SL->dados + i * n;

    return 1;
}

void liberaMatrizDensa( SistLinear_t *SL ) {
    free( SL->linhas );
    free( SL->dados );
}

/* A referência densa percorre também os zeros estruturais da Jacobiana. */
void calcJDensa( SistLinear_t *SL, const double *X ) {
    for ( size_t i = 0; i < SL->n; i++ ) {
        double *linha = SL->linhas[i];
        for ( size_t j = 0; j < SL->n; j++ ) {
            if ( i == j )
                linha[j] = 3.0 - 4.0 * X[i];
            else if ( i == j + 1 )
                linha[j] = -1.0;
            else if ( j == i + 1 )
                linha[j] = -2.0;
            else
                linha[j] = 0.0;
        }
    }
}

/* Gauss com pivoteamento parcial e retrossubstituição densos. */
int eliminacaoGauss( SistLinear_t *SL ) {
    size_t n = SL->n;
    double **a = SL->linhas;
    double *b = SL->b;

    for ( size_t k = 0; k < n; k++ ) {
        size_t pivo = k;
        double maior = fabs( a[k][k] );

        if ( !isfinite( maior ) )
            return 0;

        for ( size_t i = k + 1; i < n; i++ ) {
            double valor = fabs( a[i][k] );

            if ( !isfinite( valor ) )
                return 0;

            if ( valor > maior ) {
                maior = valor;
                pivo = i;
            }
        }

        if ( maior == 0.0 )
            return 0;

        if ( pivo != k ) {
            double *linha = a[k];
            double valor = b[k];
            a[k] = a[pivo];
            a[pivo] = linha;
            b[k] = b[pivo];
            b[pivo] = valor;
        }

        for ( size_t i = k + 1; i < n; i++ ) {
            double m = a[i][k] / a[k][k];
            a[i][k] = 0.0;
            for ( size_t j = k + 1; j < n; j++ )
                a[i][j] -= m * a[k][j];
            b[i] -= m * b[k];
        }
    }

    for ( size_t k = n; k > 0; k-- ) {
        size_t i = k - 1;
        double soma = b[i];
        for ( size_t j = i + 1; j < n; j++ )
            soma -= a[i][j] * b[j];
        b[i] = soma / a[i][i];
    }

    return 1;
}
