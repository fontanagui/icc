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

#ifdef BROYDEN_DENSA
#include "Densa.h"
#define NUM_VETORES 2
#else
#define NUM_VETORES 5
#endif

typedef enum { AVANCOU, CONVERGIU, PIVO_INVALIDO, VALOR_NAO_FINITO } Passo;

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

/* Faz um passo de Newton. O vetor b passa a conter o novo X. */
static Passo calculaPasso( const double *X, double epsilon, SistLinear_t *SL, Tempos_t *T ) {
    rtime_t inicio;

    calcMenosF( SL->b, X, SL->n );

    if ( epsilon > 0.0 ) {
        double erro = norma( SL->b, SL->n );

        if ( !isfinite( erro ) )
            return VALOR_NAO_FINITO;

        if ( erro <= epsilon )
            return CONVERGIU;
    }

    /* Calcula somente a diagonal principal da Jacobiana. */
    inicio = timestamp();
    LIKWID_MARKER_START( MARCA_JACOBIANA );
#ifdef BROYDEN_DENSA
    calcJDensa( SL, X );
#else
    calcJ( SL->principal, X, SL->n );
#endif
    LIKWID_MARKER_STOP( MARCA_JACOBIANA );
    T->jacobiana += timestamp() - inicio;

    inicio = timestamp();
    LIKWID_MARKER_START( MARCA_GAUSS );
    int resolveu = eliminacaoGauss( SL );
    LIKWID_MARKER_STOP( MARCA_GAUSS );
    T->gauss += timestamp() - inicio;

    if ( !resolveu )
        return PIVO_INVALIDO;

    if ( !atualizaX( SL->b, X, SL->n ) )
        return VALOR_NAO_FINITO;

    return AVANCOU;
}

/* Aplica o método de Newton e imprime os resultados de cada iteração. */
int newton( size_t n, double x0, double epsilon, size_t MAXIT ) {
    size_t i, tamanho;
    double *memoria, *X, *FX, *troca;
    SistLinear_t SL = { .n = n };
    Tempos_t T = {0};
    Passo resultado = AVANCOU;

    memoria = alocaVetores( n, &tamanho );
#ifdef BROYDEN_DENSA
    if ( memoria != NULL && !alocaMatrizDensa( &SL ) ) {
        free( memoria );
        memoria = NULL;
    }
#endif

    if ( memoria == NULL ) {
        fputs( "broyden: memória insuficiente para a dimensão solicitada.\n", stderr );

        return 1;
    }

    /* Os vetores de trabalho ficam no mesmo bloco contíguo. */
    X = memoria;
    FX = memoria + tamanho;
    SL.b = FX;
#ifndef BROYDEN_DENSA
    SL.principal = memoria + 2 * tamanho;
    SL.superior = memoria + 3 * tamanho;
    SL.segunda = memoria + 4 * tamanho;
#endif

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
        resultado = calculaPasso( X, epsilon, &SL, &T );
        LIKWID_MARKER_STOP( MARCA_NEWTON );
        T.total += timestamp() - inicio;

        if ( resultado != AVANCOU )
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
#ifdef BROYDEN_DENSA
    liberaMatrizDensa( &SL );
#endif
    free( memoria );

    if ( resultado == PIVO_INVALIDO ) {
        fputs( "broyden: Jacobiana singular ou pivô final inutilizável.\n", stderr );

        return 1;
    }

    if ( resultado == VALOR_NAO_FINITO ) {
        fputs( "broyden: resíduo ou atualização não finita.\n", stderr );

        return 1;
    }

    return 0;
}
