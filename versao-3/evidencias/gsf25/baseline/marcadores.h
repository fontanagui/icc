#ifndef MARCADORES_H
#define MARCADORES_H
/* make LIKWID=1 ativa a biblioteca real; build comum não requer LIKWID. */
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
#endif
