#include <stdio.h>
#include <string.h>
#include <math.h>

#include "utils.h"

/*  Retorna tempo em milisegundos desde EPOCH

    Forma de uso:
 
    rtime_t tempo;
    tempo = timestamp();
    <trecho de programa do qual se deseja medir tempo>
    tempo = timestamp() - tempo;
*/

rtime_t timestamp (void)
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
  return ( (rtime_t) tp.tv_sec*1.0e3 + (rtime_t) tp.tv_nsec*1.0e-6 );
}

/* Gera string '<baseName>_n'
 * Por exemplo, se baseName = "ABC" e n = 10,
 *  Função retorna a string "ABC_10"
 * Útil para gerar marcadores para LIKWID
 */
string_t markerName(string_t baseName, int n)
{
    int length = snprintf(NULL, 0, "%s_%d", baseName, n);
    if (length < 0) return NULL;
    string_t mark = malloc((size_t)length + 1);
    if (mark) snprintf(mark, (size_t)length + 1, "%s_%d", baseName, n);
    return mark;

}

void prnVetor(double *X, unsigned int n) {
  for (unsigned int i = 0; i < n; ++i)
    printf("%.10g ", X[i]);
  printf("\n");
}

void prnMatriz(double **M, unsigned int n) {
  for (unsigned int i = 0; i < n; ++i) {
    for (unsigned int j = 0; j < n; ++j)
      printf("%.10g ", M[i][j]);
    printf("\n");
  }
}

