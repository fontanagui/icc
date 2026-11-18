# Estado do relatório

O [artigo da revisão](experimentos/campanha-final/artigo-revisao.pdf) e a [campanha atual](experimentos/campanha-final/) apresentam medições de tempo das versões original e revisada no Intel Core i5-7500. O PDF da raiz e os CSV em `dados/` são históricos.

As sondas do LIKWID 5.4.1 compilaram os binários instrumentados e leram a topologia. A máquina bloqueou os contadores de hardware com `perf_event_paranoid=3`. Os [logs da campanha](experimentos/campanha-final/ambiente/) registram as tentativas. A análise atual tem três gráficos de tempo e um de speedup. A cobertura de hardware e a contagem oficial de gráficos dependem da campanha em ambiente com contadores e da conferência do enunciado.

A última equação segue provisoriamente com `+1`. O gerador do primeiro commit omite o termo e a cópia local o inclui. Antes da entrega, confira a figura oficial, ajuste a constante se necessário e preencha `\Autores` em `relatorio/artigo.tex`.
