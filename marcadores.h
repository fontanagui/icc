#ifndef MARCADORES_H
#define MARCADORES_H

/* Marcadores usados pelo LIKWID. O código também compila sem LIKWID. */
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
#define MARCA_GAUSS "Sistema_Linear"

#endif
