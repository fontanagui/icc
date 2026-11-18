#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NewtonSNL.h"
#include "Entrada.h"
#include "EliminacaoGauss.h"
#include "SistNaoLinear.h"
#include "marcadores.h"

#define NUM_VETORES 5

/*
 * Aloca todos os vetores em um bloco só, antes das iterações.
 * A versão otimizada usa alinhamento de 64 bytes para facilitar o SIMD.
 */
static double *alocaVetores( size_t n, size_t *tamanho ) {
    double *memoria;
    size_t bytes;

    *tamanho = n;

#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    if ( n > SIZE_MAX - 7 )
        return NULL;

    /* Arredonda o tamanho para um múltiplo de 8 doubles (64 bytes). */
    *tamanho = ( n + 7 ) / 8 * 8;
#endif

    if ( *tamanho > SIZE_MAX / ( NUM_VETORES * sizeof( double ) ) )
        return NULL;

    bytes = NUM_VETORES * ( *tamanho ) * sizeof( double );

#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    memoria = aligned_alloc( ALINHAMENTO, bytes );
#else
    memoria = malloc( bytes );
#endif

    if ( memoria == NULL )
        return NULL;

    memset( memoria, 0, bytes );

    return memoria;
}

/*
 * Faz uma iteração de Newton:
 * 0: atualizou X; 1: convergiu; 2: erro no sistema; 3: erro numérico.
 */
static int calculaPasso( const double *X, double *FX, double epsilon,
                        SistLinear_t *SL, Tempos_t *T ) {
    size_t i;
    rtime_t inicio;

    calcF( FX, X, SL->n );

    if ( epsilon > 0.0 ) {
        double erro = norma( FX, SL->n );

        if ( !isfinite( erro ) )
            return 3;

        if ( erro <= epsilon )
            return 1;
    }

    /* Calcula somente a diagonal principal da Jacobiana. */
    inicio = timestamp();
    LIKWID_MARKER_START( MARCA_JACOBIANA );
    calcJ( SL->principal, X, SL->n );
    LIKWID_MARKER_STOP( MARCA_JACOBIANA );
    T->jacobiana += timestamp() - inicio;

    /* Para obter o passo S, resolvemos J*S = -F(X). */
    for ( i = 0; i < SL->n; i++ )
        FX[i] = -FX[i];

    inicio = timestamp();
    LIKWID_MARKER_START( MARCA_GAUSS );
    int resolveu = eliminacaoGauss( SL );
    LIKWID_MARKER_STOP( MARCA_GAUSS );
    T->gauss += timestamp() - inicio;

    if ( !resolveu )
        return 2;

    if ( !atualizaX( FX, X, SL->n ) )
        return 3;

    return 0;
}

/* Aplica o método de Newton e imprime os resultados de cada iteração. */
int newton( size_t n, double x0, double epsilon, size_t MAXIT ) {
    size_t i, tamanho;
    double *memoria, *X, *FX, *troca;
    SistLinear_t SL;
    Tempos_t T = {0};
    int resultado = 0;

    memoria = alocaVetores( n, &tamanho );

    if ( memoria == NULL ) {
        fputs( "broyden: memória insuficiente para a dimensão solicitada.\n", stderr );

        return 1;
    }

    /* Os cinco vetores compartilham a mesma região contínua da memória. */
    X = memoria;
    FX = memoria + tamanho;
    SL.n = n;
    SL.principal = memoria + 2 * tamanho;
    SL.inferior = memoria + 3 * tamanho;
    SL.superior = memoria + 4 * tamanho;
    SL.b = FX;

    for ( i = 0; i < n; i++ )
        X[i] = x0;

    imprimeX( X, n, 1 );

    LIKWID_MARKER_INIT;
    LIKWID_MARKER_THREADINIT;
    LIKWID_MARKER_REGISTER( MARCA_NEWTON );
    LIKWID_MARKER_REGISTER( MARCA_JACOBIANA );
    LIKWID_MARKER_REGISTER( MARCA_GAUSS );

    for ( i = 0; i < MAXIT; i++ ) {
        rtime_t inicio = timestamp();

        LIKWID_MARKER_START( MARCA_NEWTON );
        resultado = calculaPasso( X, FX, epsilon, &SL, &T );
        LIKWID_MARKER_STOP( MARCA_NEWTON );
        T.total += timestamp() - inicio;

        if ( resultado != 0 )
            break;

        /* FX recebeu X + S; trocar ponteiros evita copiar o vetor todo. */
        troca = X;
        X = FX;
        FX = troca;
        SL.b = FX;

        T.passos++;
        imprimeX( X, n, 0 );
    }

    LIKWID_MARKER_CLOSE;
    imprimeTempos( &T );
    free( memoria );

    if ( resultado == 2 ) {
        fputs( "broyden: Jacobiana singular ou pivô final inutilizável.\n", stderr );

        return 1;
    }

    if ( resultado == 3 ) {
        fputs( "broyden: resíduo ou atualização não finita.\n", stderr );

        return 1;
    }

    return 0;
}
