#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Entrada.h"

#define TAM_PALAVRA 128

/* Lê uma palavra da entrada, conferindo seu tamanho. */
static int lePalavra( char palavra[TAM_PALAVRA] ) {
    int c;
    size_t i = 0;

    do {
        c = getchar();
    } while ( c != EOF && isspace( c ) );

    if ( c == EOF )
        return 0;

    while ( c != EOF && !isspace( c ) ) {
        if ( c == '\0' || i == TAM_PALAVRA - 1 )
            return 0;

        palavra[i++] = (char)c;
        c = getchar();
    }

    palavra[i] = '\0';

    return i > 0;
}

/* Converte uma palavra para um inteiro não negativo. */
static int leInteiro( size_t *numero ) {
    char palavra[TAM_PALAVRA];
    char *fim;
    uintmax_t valor;

    if ( !lePalavra( palavra ) )
        return 0;

    if ( palavra[0] == '-' )
        return 0;

    errno = 0;
    valor = strtoumax( palavra, &fim, 10 );

    if ( errno == ERANGE || fim == palavra || *fim != '\0' || valor > SIZE_MAX )
        return 0;

    *numero = (size_t)valor;

    return 1;
}

/* Converte uma palavra para um número real finito. */
static int leReal( double *numero ) {
    char palavra[TAM_PALAVRA];
    char *fim;
    double valor;

    if ( !lePalavra( palavra ) )
        return 0;

    errno = 0;
    valor = strtod( palavra, &fim );

    if ( fim == palavra || *fim != '\0' || !isfinite( valor ) )
        return 0;

    if ( errno == ERANGE && valor == 0.0 )
        return 0;

    *numero = valor;

    return 1;
}

/* Lê os quatro dados solicitados no enunciado. */
int leEntrada( size_t *n, double *x0, double *epsilon, size_t *MAXIT ) {
    if ( !leInteiro( n ) || *n == 0 )
        return 0;

    if ( !leReal( x0 ) )
        return 0;

    if ( !leReal( epsilon ) || *epsilon < 0.0 )
        return 0;

    return leInteiro( MAXIT );
}

/* Imprime as variáveis de uma iteração. */
void imprimeX( const double *X, size_t n, int primeiro ) {
    size_t i;

    if ( !primeiro )
        puts( "#" );

    for ( i = 0; i < n; i++ )
        printf( "x%zu = %.17g\n", i + 1, X[i] );
}

/* Os rótulos ficam no stderr, sem alterar a saída exigida. */
void imprimeTempos( const Tempos_t *T ) {
    const char *coleta;

    printf( "###########\n%.12g\n%.12g\n%.12g\n###########\n",
            T->total, T->jacobiana, T->gauss );

    coleta = getenv( "BROYDEN_STATS" );

    if ( coleta != NULL && strcmp( coleta, "1" ) == 0 ) {
        fprintf( stderr, "BROYDEN_STATS,%zu,%.12g,%.12g,%.12g\n",
                T->passos, T->total, T->jacobiana, T->gauss );
    }
}
