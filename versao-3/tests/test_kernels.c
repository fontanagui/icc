#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "SistNaoLinear.h"
#include "SistLinear.h"
#include "EliminacaoGauss.h"

#define N 5

/* Confere a Jacobiana comparando suas derivadas com diferenças centrais. */
static int testaJacobiana( void ) {
    size_t i, j;
    const double h = 1e-5;
    _Alignas(64) double X[N] = {-0.7, 0.3, -1.1, 0.6, -0.2};
    _Alignas(64) double diagonal[N];
    double mais[N], menos[N];

    calcJ( diagonal, X, N );

    for ( j = 0; j < N; j++ ) {
        double antigo = X[j];

        X[j] = antigo + h;
        calcMenosF( mais, X, N );
        X[j] = antigo - h;
        calcMenosF( menos, X, N );
        X[j] = antigo;

        for ( i = 0; i < N; i++ ) {
            double esperado = 0.0;
            double derivada = ( menos[i] - mais[i] ) / ( 2.0 * h );

            if ( i == j )
                esperado = diagonal[i];
            else if ( i + 1 == j )
                esperado = -2.0;
            else if ( j + 1 == i )
                esperado = -1.0;

            if ( !isfinite( derivada ) || fabs( derivada - esperado ) > 1e-9 )
                return 0;
        }
    }

    return 1;
}

/* Monta um sistema com solução conhecida para testar a eliminação de Gauss. */
static int testaGauss( const double *a, size_t n ) {
    size_t i;
    const double esperado[N] = {2.0, -1.0, 0.5, -3.0, 4.0};
    double principal[N], superior[N], segunda[N], b[N];
    SistLinear_t SL = { .n = n, .b = b, .principal = principal, .superior = superior, .segunda = segunda };

    for ( i = 0; i < n; i++ ) {
        principal[i] = a[i];
        b[i] = a[i] * esperado[i];

        if ( i > 0 )
            b[i] -= esperado[i - 1];

        if ( i + 1 < n )
            b[i] -= 2.0 * esperado[i + 1];
    }

    if ( !eliminacaoGauss( &SL ) )
        return 0;

    for ( i = 0; i < n; i++ ) {
        if ( !isfinite( b[i] ) || fabs( b[i] - esperado[i] ) > 1e-12 )
            return 0;
    }

    return 1;
}

static int testaPivoFinal( void ) {
    double principal[2] = {1.0, 2.0}, superior[2], segunda[2], b[2] = {0.0, 0.0};
    SistLinear_t SL = { .n = 2, .b = b, .principal = principal, .superior = superior, .segunda = segunda };

    /* O determinante da matriz de ordem dois é zero. */
    if ( eliminacaoGauss( &SL ) )
        return 0;

    /* Uma divisão direta também resolve um pivô subnormal. */
    SL.n = 1;
    principal[0] = DBL_TRUE_MIN;
    b[0] = DBL_TRUE_MIN;

    return eliminacaoGauss( &SL ) && b[0] == 1.0;
}

int main( void ) {
    const double dominante[N] = {-3.0, 3.0, -3.0, 3.0, -3.0};
    const double comTrocas[N] = {0.0, 2.0, 0.0, 2.0, 3.0};
    const double quaseZero[N] = {1e-16, 2.0, 1e-16, -2.0, 3.0};

    for ( size_t n = 1; n <= N; n++ ) {
        if ( !testaGauss( dominante, n ) ) {
            fprintf( stderr, "Falha no sistema dominante de dimensão %zu.\n", n );

            return EXIT_FAILURE;
        }
    }

    if ( !testaJacobiana() || !testaGauss( comTrocas, N ) ||
         !testaGauss( quaseZero, N ) || !testaPivoFinal() ) {
        fputs( "Falha nos testes de Jacobiana e Gauss.\n", stderr );

        return EXIT_FAILURE;
    }

    puts( "Jacobiana e sistemas lineares conferidos com sucesso." );

    return EXIT_SUCCESS;
}
