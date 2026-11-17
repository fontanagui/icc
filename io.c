#include "io.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { CAPACIDADE_TOKEN = 128 };

/* Lê um token inteiro. Um token longo demais é consumido e rejeitado. */
static bool le_token(char token[CAPACIDADE_TOKEN])
{
    size_t comprimento = 0;
    int c;

    do {
        c = getchar();
    } while (c != EOF && isspace(c));

    while (c != EOF && !isspace(c)) {
        if (comprimento < CAPACIDADE_TOKEN - 1) {
            token[comprimento] = (char)c;
        }
        if (comprimento < CAPACIDADE_TOKEN) {
            ++comprimento;
        }
        c = getchar();
    }

    if (comprimento == 0 || comprimento >= CAPACIDADE_TOKEN) {
        return false;
    }

    token[comprimento] = '\0';
    return memchr(token, '\0', comprimento) == NULL;
}

/* Converte um inteiro não negativo que caiba em size_t. */
static bool le_tamanho(size_t *valor)
{
    char token[CAPACIDADE_TOKEN];
    char *fim;

    if (!le_token(token) || token[0] == '-') {
        return false;
    }

    errno = 0;
    const uintmax_t numero = strtoumax(token, &fim, 10);
    if (errno == ERANGE || fim == token || *fim != '\0' || numero > SIZE_MAX) {
        return false;
    }

    *valor = (size_t)numero;
    return true;
}

/* Converte um real finito e preserva valores subnormais representáveis. */
static bool le_real(double *valor)
{
    char token[CAPACIDADE_TOKEN];
    char *fim;

    if (!le_token(token)) {
        return false;
    }

    errno = 0;
    const double numero = strtod(token, &fim);
    if (fim == token || *fim != '\0' || !isfinite(numero)
        || (errno == ERANGE && numero == 0.0)) {
        return false;
    }

    *valor = numero;
    return true;
}

/* Aceita os quatro parâmetros separados por espaços ou quebras de linha. */
bool le_problema(Problema *problema)
{
    return le_tamanho(&problema->n) && problema->n > 0
        && le_real(&problema->x0)
        && le_real(&problema->epsilon) && problema->epsilon >= 0.0
        && le_tamanho(&problema->max);
}

/* Escreve um vetor com precisão suficiente para ler de volta cada double. */
void imprime_iteracao(size_t n, const double *x, bool primeira)
{
    if (!primeira) {
        puts("#");
    }

    for (size_t i = 0; i < n; ++i) {
        printf("x%zu = %.17g\n", i + 1, x[i]);
    }
}

/* Mantém o rodapé especificado e fornece a contagem ao coletor via stderr. */
void imprime_medidas(const Medidas *medidas)
{
    printf("###########\n%.12g\n%.12g\n%.12g\n###########\n",
           medidas->total, medidas->jacobianas, medidas->sistemas);

    const char *coleta = getenv("BROYDEN_STATS");
    if (coleta != NULL && strcmp(coleta, "1") == 0) {
        fprintf(stderr, "BROYDEN_STATS,%zu,%.12g,%.12g,%.12g\n",
                medidas->passos, medidas->total, medidas->jacobianas, medidas->sistemas);
    }
}
