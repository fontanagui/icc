#include "newton.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "broyden.h"
#include "io.h"
#include "jacobiana.h"
#include "sistema_linear.h"

#ifdef LIKWID_PERFMON
#include <likwid-marker.h>
#else
#define LIKWID_MARKER_INIT ((void)0)
#define LIKWID_MARKER_THREADINIT ((void)0)
#define LIKWID_MARKER_REGISTER(regiao) ((void)0)
#define LIKWID_MARKER_START(regiao) ((void)0)
#define LIKWID_MARKER_STOP(regiao) ((void)0)
#define LIKWID_MARKER_CLOSE ((void)0)
#endif

#define MARCA_NEWTON "Newton_Total"
#define MARCA_JACOBIANA "Jacobiana"
#define MARCA_SISTEMA "Sistema_Linear"

enum EstadoPasso {
    PASSO_CALCULADO,
    RESIDUO_ACEITO,
    JACOBIANA_SINGULAR,
    VALOR_NAO_FINITO
};

enum { NUM_VETORES = 5 };

/* Reserva cinco fatias em uma alocação e inicializa as páginas antes da coleta. */
static double *aloca_trabalho(size_t n, size_t *passo)
{
    size_t comprimento = n;
#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    const size_t faixa = BROYDEN_ALINHAMENTO / sizeof(double);
    if (n > SIZE_MAX - (faixa - 1)) {
        return NULL;
    }
    comprimento = (n + faixa - 1) / faixa * faixa;
#endif
    if (comprimento > SIZE_MAX / (NUM_VETORES * sizeof(double))) {
        return NULL;
    }

    const size_t bytes = comprimento * NUM_VETORES * sizeof(double);
#if !defined(BROYDEN_BASELINE) && !defined(BROYDEN_SEM_ALINHAMENTO)
    double *memoria = aligned_alloc(BROYDEN_ALINHAMENTO, bytes);
#else
    double *memoria = malloc(bytes);
#endif
    if (memoria != NULL) {
        memset(memoria, 0, bytes);
        *passo = comprimento;
    }
    return memoria;
}

/* Calcula o candidato a próximo X. Os temporizadores externos incluem o resíduo. */
static enum EstadoPasso calcula_passo(const Problema *problema, const double *x,
                                      double *rhs, Tridiagonal j, Medidas *medidas)
{
    const size_t n = problema->n;
    broyden_rhs(n, x, rhs);
    if (problema->epsilon > 0.0) {
        const double residuo = norma_inf(n, rhs);
        if (!isfinite(residuo)) {
            return VALOR_NAO_FINITO;
        }
        if (residuo <= problema->epsilon) {
            return RESIDUO_ACEITO;
        }
    }

    rtime_t inicio = timestamp();
    LIKWID_MARKER_START(MARCA_JACOBIANA);
    constroi_jacobiana(n, x, j);
    LIKWID_MARKER_STOP(MARCA_JACOBIANA);
    medidas->jacobianas += timestamp() - inicio;

    inicio = timestamp();
    LIKWID_MARKER_START(MARCA_SISTEMA);
    const bool resolvido = resolve_sistema(n, j, rhs);
    LIKWID_MARKER_STOP(MARCA_SISTEMA);
    medidas->sistemas += timestamp() - inicio;

    if (!resolvido) {
        return JACOBIANA_SINGULAR;
    }
    return aplica_passo(n, x, rhs) ? PASSO_CALCULADO : VALOR_NAO_FINITO;
}

/* Executa e imprime cada passo. Os marcadores acumulam apenas trechos de cálculo. */
const char *executa_newton(const Problema *problema)
{
    size_t passo;
    double *memoria = aloca_trabalho(problema->n, &passo);
    if (memoria == NULL) {
        return "memória insuficiente para a dimensão solicitada.";
    }

    double *x = memoria;
    double *rhs = memoria + passo;
    const Tridiagonal j = {memoria + 2 * passo, memoria + 3 * passo, memoria + 4 * passo};
    for (size_t i = 0; i < problema->n; ++i) {
        x[i] = problema->x0;
    }
    imprime_iteracao(problema->n, x, true);

    Medidas medidas = {0};
    enum EstadoPasso estado = PASSO_CALCULADO;
    LIKWID_MARKER_INIT;
    LIKWID_MARKER_THREADINIT;
    LIKWID_MARKER_REGISTER(MARCA_NEWTON);
    LIKWID_MARKER_REGISTER(MARCA_JACOBIANA);
    LIKWID_MARKER_REGISTER(MARCA_SISTEMA);

    while (medidas.passos < problema->max) {
        const rtime_t inicio = timestamp();
        LIKWID_MARKER_START(MARCA_NEWTON);
        estado = calcula_passo(problema, x, rhs, j, &medidas);
        LIKWID_MARKER_STOP(MARCA_NEWTON);
        medidas.total += timestamp() - inicio;

        if (estado != PASSO_CALCULADO) {
            break;
        }

        /* A próxima iteração reutiliza o X anterior como área de trabalho. */
        double *anterior = x;
        x = rhs;
        rhs = anterior;
        ++medidas.passos;
        imprime_iteracao(problema->n, x, false);
    }

    LIKWID_MARKER_CLOSE;
    imprime_medidas(&medidas);
    free(memoria);

    if (estado == JACOBIANA_SINGULAR) {
        return "Jacobiana singular ou pivô final inutilizável.";
    }
    if (estado == VALOR_NAO_FINITO) {
        return "resíduo ou atualização não finita.";
    }
    return NULL;
}
