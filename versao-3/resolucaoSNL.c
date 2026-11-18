#include <stdio.h>
#include <stdlib.h>
#include "Entrada.h"
#include "NewtonSNL.h"

int main( void ) {
    size_t n, MAXIT;
    double x0, epsilon;

    if ( !leEntrada( &n, &x0, &epsilon, &MAXIT ) ) {
        fputs( "Entrada inválida. Informe N > 0, x0 finito, epsilon >= 0 e MAX >= 0.\n", stderr );

        return EXIT_FAILURE;
    }

    if ( newton( n, x0, epsilon, MAXIT ) != 0 )
        return EXIT_FAILURE;

    if ( fflush( stdout ) == EOF || ferror( stdout ) ) {
        fputs( "broyden: falha ao escrever a saída.\n", stderr );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
