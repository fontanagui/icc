#ifndef NEWTON_SNL_H
#define NEWTON_SNL_H
#include <stddef.h>
/* Histórico contíguo: estado inicial seguido de cada atualização de Newton. */
typedef struct {
    size_t n, iteracoes;
    double *historico;
    double total_ms, jacobiana_ms, linear_ms;
} ResultadoNewton;
/* epsilon=0 desativa a parada por tolerância. Retorna -1 se falhar. */
int newton(size_t n, double x0, double epsilon, size_t maxit, ResultadoNewton *r);
/* Imprime o histórico fora das regiões medidas, seguido dos três tempos em ms. */
void imprimeResultado(const ResultadoNewton *r);
void liberaResultado(ResultadoNewton *r);
#endif
