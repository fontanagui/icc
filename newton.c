#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "newton.h"
#include "broyden.h"
#include "io.h"
#include "jacobiana.h"
#include "sistema_linear.h"

/* O mesmo código funciona com ou sem a biblioteca LIKWID instalada. */
#ifdef LIKWID_PERFMON
#include <likwid-marker.h>
#else
#define LIKWID_MARKER_INIT ((void)0)
#define LIKWID_MARKER_THREADINIT ((void)0)
#define LIKWID_MARKER_REGISTER(nome) ((void)0)
#define LIKWID_MARKER_START(nome) ((void)0)
#define LIKWID_MARKER_STOP(nome) ((void)0)
#define LIKWID_MARKER_CLOSE ((void)0)
#endif

#define MARCA_NEWTON "Newton_Total"
#define MARCA_JACOBIANA "Jacobiana"
#define MARCA_SISTEMA "Sistema_Linear"

/* Cada situação tem um resultado diferente para o laço de Newton. */
enum ResultadoPasso {
    PASSO_OK,
    CONVERGIU,
    ERRO_SISTEMA,
    ERRO_NUMERICO
};

#define NUM_VETORES 5

/*
 * Aloca cinco vetores de uma só vez:
 * x, rhs, diagonal principal, diagonal inferior e diagonal superior.
 * Na versão otimizada, cada vetor começa num endereço múltiplo de 64.
 */
static double *aloca_vetores(size_t n, size_t *tamVetor)
{
    size_t tamanho = n;

#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    size_t bloco = BROYDEN_ALINHAMENTO / sizeof(double);
    if (n > SIZE_MAX - (bloco - 1))
        return NULL;
    tamanho = (n + bloco - 1) / bloco * bloco;
#endif

    if (tamanho > SIZE_MAX / (NUM_VETORES * sizeof(double)))
        return NULL;

    size_t bytes = tamanho * NUM_VETORES * sizeof(double);
    double *memoria;

#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    memoria = aligned_alloc(BROYDEN_ALINHAMENTO, bytes);
#else
    memoria = malloc(bytes);
#endif

    if (memoria == NULL)
        return NULL;

    /* Inicializa uma vez. Não haverá malloc dentro de Newton. */
    memset(memoria, 0, bytes);
    *tamVetor = tamanho;
    return memoria;
}

/*
 * Faz uma atualização de Newton:
 * 1) calcula -F(x); 2) monta a Jacobiana;
 * 3) resolve J*passo = -F(x); 4) soma o passo ao x.
 */
static enum ResultadoPasso calcula_passo(const Problema *problema, const double *x,
                                        double *rhs, Tridiagonal j, Medidas *medidas)
{
    size_t n = problema->n;

    broyden_rhs(n, x, rhs);

    /* Com epsilon = 0, executamos as MAX atualizações para o benchmark. */
    if (problema->epsilon > 0.0) {
        double erro = norma_inf(n, rhs);
        if (!isfinite(erro))
            return ERRO_NUMERICO;
        if (erro <= problema->epsilon)
            return CONVERGIU;
    }

    rtime_t inicio = timestamp();
    LIKWID_MARKER_START(MARCA_JACOBIANA);
    constroi_jacobiana(n, x, j);
    LIKWID_MARKER_STOP(MARCA_JACOBIANA);
    medidas->jacobianas += timestamp() - inicio;

    inicio = timestamp();
    LIKWID_MARKER_START(MARCA_SISTEMA);
    bool resolveu = resolve_sistema(n, j, rhs);
    LIKWID_MARKER_STOP(MARCA_SISTEMA);
    medidas->sistemas += timestamp() - inicio;

    if (!resolveu)
        return ERRO_SISTEMA;
    if (!aplica_passo(n, x, rhs))
        return ERRO_NUMERICO;

    return PASSO_OK;
}

/* Resolve Broyden pelo método de Newton e imprime cada vetor encontrado. */
const char *executa_newton(const Problema *problema)
{
    size_t tamVetor;
    double *memoria = aloca_vetores(problema->n, &tamVetor);
    if (memoria == NULL)
        return "memória insuficiente para a dimensão solicitada.";

    /* Cinco vetores dentro do mesmo bloco de memória. */
    double *x = memoria;
    double *rhs = memoria + tamVetor;
    Tridiagonal j = {
        memoria + 2 * tamVetor,
        memoria + 3 * tamVetor,
        memoria + 4 * tamVetor
    };

    for (size_t i = 0; i < problema->n; i++)
        x[i] = problema->x0;

    imprime_iteracao(problema->n, x, true);

    Medidas medidas = {0};
    enum ResultadoPasso resultado = PASSO_OK;

    LIKWID_MARKER_INIT;
    LIKWID_MARKER_THREADINIT;
    LIKWID_MARKER_REGISTER(MARCA_NEWTON);
    LIKWID_MARKER_REGISTER(MARCA_JACOBIANA);
    LIKWID_MARKER_REGISTER(MARCA_SISTEMA);

    while (medidas.passos < problema->max) {
        rtime_t inicio = timestamp();
        LIKWID_MARKER_START(MARCA_NEWTON);
        resultado = calcula_passo(problema, x, rhs, j, &medidas);
        LIKWID_MARKER_STOP(MARCA_NEWTON);
        medidas.total += timestamp() - inicio;

        if (resultado != PASSO_OK)
            break;

        /* rhs agora contém o novo x. Trocamos os ponteiros, sem copiar vetores. */
        double *anterior = x;
        x = rhs;
        rhs = anterior;

        medidas.passos++;
        imprime_iteracao(problema->n, x, false);
    }

    LIKWID_MARKER_CLOSE;
    imprime_medidas(&medidas);
    free(memoria);

    if (resultado == ERRO_SISTEMA)
        return "Jacobiana singular ou pivô final inutilizável.";
    if (resultado == ERRO_NUMERICO)
        return "resíduo ou atualização não finita.";
    return NULL;
}
