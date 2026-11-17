#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"

#define TAM_TOKEN 128

/* Lê uma palavra da entrada. Rejeita valores grandes demais. */
static bool le_token(char token[TAM_TOKEN])
{
    size_t tam = 0;
    int c;

    do {
        c = getchar();
    } while (c != EOF && isspace(c));

    while (c != EOF && !isspace(c)) {
        if (tam < TAM_TOKEN - 1)
            token[tam] = (char)c;
        if (tam < TAM_TOKEN)
            tam++;
        c = getchar();
    }

    if (tam == 0 || tam >= TAM_TOKEN)
        return false;

    token[tam] = '\0';
    return memchr(token, '\0', tam) == NULL;
}

/* Lê um número inteiro não negativo. */
static bool le_inteiro(size_t *numero)
{
    char token[TAM_TOKEN];
    char *fim;

    if (!le_token(token) || token[0] == '-')
        return false;

    errno = 0;
    uintmax_t valor = strtoumax(token, &fim, 10);
    if (errno == ERANGE || fim == token || *fim != '\0' || valor > SIZE_MAX)
        return false;

    *numero = (size_t)valor;
    return true;
}

/* Lê um double válido, sem aceitar NaN ou infinito. */
static bool le_real(double *numero)
{
    char token[TAM_TOKEN];
    char *fim;

    if (!le_token(token))
        return false;

    errno = 0;
    double valor = strtod(token, &fim);
    if (fim == token || *fim != '\0' || !isfinite(valor)
        || (errno == ERANGE && valor == 0.0))
        return false;

    *numero = valor;
    return true;
}

/* Lê N, x0, epsilon e MAX, nessa ordem. */
bool le_problema(Problema *problema)
{
    return le_inteiro(&problema->n) && problema->n > 0
        && le_real(&problema->x0)
        && le_real(&problema->epsilon) && problema->epsilon >= 0.0
        && le_inteiro(&problema->max);
}

/* Imprime cada incógnita em uma linha e separa as iterações com #. */
void imprime_iteracao(size_t n, const double *x, bool primeira)
{
    if (!primeira)
        puts("#");

    for (size_t i = 0; i < n; i++)
        printf("x%zu = %.17g\n", i + 1, x[i]);
}

/* Imprime os três tempos no formato que o professor pediu. */
void imprime_medidas(const Medidas *medidas)
{
    printf("###########\n%.12g\n%.12g\n%.12g\n###########\n",
           medidas->total, medidas->jacobianas, medidas->sistemas);

    /* Opcional: o script de benchmark lê a contagem via stderr. */
    const char *coleta = getenv("BROYDEN_STATS");
    if (coleta != NULL && strcmp(coleta, "1") == 0) {
        fprintf(stderr, "BROYDEN_STATS,%zu,%.12g,%.12g,%.12g\n",
                medidas->passos, medidas->total, medidas->jacobianas, medidas->sistemas);
    }
}
