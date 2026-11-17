#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "newton.h"

/* Lê o problema, executa Newton e informa falhas pelo código de saída. */
int main(void)
{
    Problema problema;

    if (!le_problema(&problema)) {
        fputs("Entrada inválida. Informe N > 0, x0 finito, epsilon >= 0 e MAX >= 0.\n",
              stderr);
        return EXIT_FAILURE;
    }

    const char *erro = executa_newton(&problema);
    if (erro != NULL) {
        fprintf(stderr, "broyden: %s\n", erro);
        return EXIT_FAILURE;
    }

    if (fflush(stdout) == EOF || ferror(stdout)) {
        fputs("broyden: falha ao escrever a saída.\n", stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
