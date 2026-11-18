# Defesa técnica

## 1. Como sai o primeiro passo

Para a função clássica, com $N=3$ e valor inicial igual a $-1$, temos $F=(-2,-1,-3)^T$. A diagonal da Jacobiana vale 7. A resolução de $J\Delta X=-F$ dá $\Delta X=(8/21,1/3,10/21)^T$. A primeira iteração impressa após o vetor inicial é aproximadamente $(-0{,}61905, -0{,}66667, -0{,}52381)$.

A última constante deve ser conferida na imagem do Moodle. A expressão do script fornecido omite o `+1` na última equação. A opção `LAST_CONSTANT=0` reproduz essa variante, em que $F_3=-4$ no exemplo. A derivada de uma constante é zero, então a Jacobiana é idêntica nas duas formulações para o mesmo vetor $X$.

## 2. Derivação e representação

Expanda o termo local para $3x_i-2x_i^2$. A derivada em relação a $x_i$ vale $3-4x_i$. Derivar as contribuições dos vizinhos dá $-1$ à esquerda e $-2$ à direita. Todas as outras derivadas valem zero. As extremidades usam vizinhos fictícios fixos em zero. Para $N=1$, vale apenas $(3-2x_1)x_1+c_1$.

Em `NewtonSNL.c`, uma alocação contém cinco fatias contíguas. Duas se alternam entre o iterado corrente e a área para resíduo, correção e novo iterado. Outra contém a diagonal principal e serve de área para os pivôs ou coeficientes de retrocessão. As duas restantes contêm as diagonais laterais quando usadas. Para $N=20000$, são 800.000 bytes reservados. O caminho de Thomas percorre três fatias, com 480.000 bytes de dados distintos. Uma matriz densa exigiria 3,2 GB só para a Jacobiana.

## 3. O sistema linear

Na baseline, `triang` e `retrossubs` escolhe entre duas linhas adjacentes a cada passo. Se o elemento abaixo do pivô for maior em módulo, realiza a troca. A troca pode criar um coeficiente em $J_{i,i+2}$. O código reutiliza a posição liberada da subdiagonal para guardá-lo até a retrocessão. A escolha do maior módulo limita o tamanho do multiplicador de eliminação. Um pivô final inválido gera erro.

No caminho de Thomas, escreva $d_i=3-4x_i$, $b=-F$ e $p_i=d_i-2/p_{i-1}$. O primeiro pivô é $p_1=d_1$. Se todo $|d_i|\ge3$, por indução todo $|p_i|\ge2$. Assim, o caminho pode calcular $r_i=1/p_i$ uma vez por linha e usar esse recíproco para o lado direito reduzido $w_i=(b_i+w_{i-1})r_i$, com $w_1=b_1r_1$. Na volta, $\Delta x_i=w_i+2r_i\Delta x_{i+1}$.

O programa usa pivotamento se a condição suficiente falhar. Em `N=2` com entrada `x0=0.75`, as duas entradas diagonais iniciais valem zero. A matriz é invertível, com determinante $-2$. O algoritmo troca as duas linhas e resolve o passo. Em `N=3` com a mesma entrada, o determinante é zero. A falha aparece em `stderr` e o processo termina com código não zero.

Tanto a eliminação quanto a retrocessão percorrem cada linha uma vez. Cada linha tem um número constante de operações. Avaliação de $F$, montagem da Jacobiana e atualização de $X$ também são lineares. Para $k$ passos, o custo é $\Theta(kN)$ e a memória de trabalho é $\Theta(N)$.

## 4. Onde o código ganha tempo

| Função | Decisão |
|---|---|
| `calcF()` | O laço interno da otimizada tem dois vizinhos válidos, sem teste de borda por elemento. As duas extremidades são tratadas fora do laço. |
| `calcJ()` | A otimizada escreve $N$ derivadas na região medida. As laterais ficam implícitas em Thomas e só são materializadas quando há pivotamento. |
| `eliminacaoGauss()` | A verificação de dominância é vetorizável. O recíproco de cada pivô serve à eliminação e à retrocessão, o que reduz as divisões. |
| `atualizaX()` | A mesma passagem soma a correção e reduz um indicador inteiro de finitude. A troca de ponteiros evita copiar todo o vetor no fim do passo. |
| `alocaVetores()` | Cada fatia da otimizada começa em um endereço múltiplo de 64 bytes. Os buffers disjuntos permitem declarar `restrict` nos kernels. |

O GCC 14.2 informou vetores de 32 e 64 bytes nos laços do resíduo interno, na diagonal principal, na checagem de dominância e na atualização. A inspeção de `jacobiana.o` mostrou `vmovapd` para acessos alinhados e de `sistema_linear.o` mostrou divisões escalares `vdivsd`. O pivô da linha seguinte usa o pivô calculado na linha anterior. Na retrocessão, a linha anterior usa a correção da linha seguinte. Essas dependências impedem aplicar diretamente SIMD em quatro ou oito linhas independentes. O código deixa os laços independentes com a vetorização automática do compilador.

O Makefile permite a contração com `-ffp-contract=fast`. O compilador pode fundir multiplicação e soma em uma instrução nas arquiteturas com FMA. O ensaio isolado mostrou queda no tempo do sistema linear, e a regressão confirmou que as diferenças de arredondamento preservaram a precisão observada.

## 5. O que cada gráfico mede

Os três marcadores são `Newton_Total`, `Jacobiana` e `Sistema_Linear`. Os dois últimos ficam dentro do primeiro. Cada um é iniciado e encerrado uma vez por atualização no ensaio com `epsilon=0`. O LIKWID acumula as chamadas de um mesmo marcador. Na medição de tempo, `timestamp()` fornece a duração acumulada das 25 atualizações em milissegundos. A impressão é chamada entre os intervalos.

Leia as quatro linhas da tabela para cada uma das três regiões. O roteiro cobre as doze figuras.

| Medida | Newton total | Jacobiana | Sistema linear |
|---|---|---|---|
| Tempo (ms) | Soma as etapas computacionais. O sistema linear pode dominar a curva. | Laço curto. Para $N$ pequeno, relógio e chamada têm peso maior. | Compare a taxa de crescimento com $N$ e a diferença entre uma e duas divisões por linha. |
| Banda (MB/s) | Reúne passagens por $X$, resíduo e dados da Jacobiana. | Compare as gravações de três vetores com as de um vetor. | O caminho de Thomas usa a diagonal e o lado direito. A banda é taxa, calculada com a duração da região. |
| Taxa de faltas | Examine onde o conjunto de trabalho pode deixar de caber no cache medido. | Cada fatia adicional da baseline ocupa linhas de cache. | A passagem reversa depende do reuso dos dados da passagem de ida. |
| MFLOP/s | Resíduo, Jacobiana e atualização contribuem com laços vetoriais. | As derivadas da diagonal são independentes. | A checagem de dominância admite SIMD. As recorrências de solução usam aritmética escalar. |

A aceleração é $T_B/T_O$, onde cada $T$ é a média da respectiva versão. Em dimensões pequenas, leia a dispersão antes de atribuir importância a uma diferença. O ensaio preliminar inclui três gráficos de tempo. As análises de banda, cache e FLOPS dependem da campanha com contadores. `FLOPS_DP` e `FLOPS_AVX` representam métricas próprias, registradas separadamente no CSV. Confira os eventos e as larguras de vetor com `likwid-perfctr -H -g GRUPO`.

## 6. Perguntas prováveis

**Por que testar o resíduo antes de resolver?** Um iterado que já satisfaz $\|F\|_\infty\le\epsilon$ pode ser aceito sem construir uma Jacobiana adicional. Um passo pequeno, isoladamente, também pode ocorrer por arredondamento ou estagnação.

**Por que o programa faz as 25 atualizações mesmo perto da raiz?** O valor `epsilon=0` desabilita a parada por tolerância. Com isso, a campanha mede a mesma quantidade de passos em cada versão.

**Por que construir a diagonal a cada passo?** Os valores de $X$ mudam e $d_i=3-4x_i$ depende do novo iterado. As diagonais laterais são constantes antes da eliminação. O algoritmo pivotado pode alterá-las durante a fatoração.

**Qual caso prova que Thomas sem teste seria inadequado?** Para $N=2$ e `x0=0.75`, o primeiro pivô nominal vale zero e a matriz é invertível. A troca de linhas resolve o passo.

**Como você verificou que o vetor está correto?** A regressão testa $J\Delta X+F(X)$ para cada passo pequeno, compara com eliminação densa independente e calcula o resíduo final para os tamanhos da campanha. Há um teste separado para a Jacobiana por diferenças centrais.

**Como você interpreta aumento de banda com redução de tempo?** A banda divide o volume pela duração. O resultado pode subir com volume similar se a duração diminuir. O volume e os eventos da região ajudam a distinguir os efeitos.

**Com mais MFLOP/s, o programa é necessariamente mais rápido?** A taxa mede operações por unidade de tempo. Uma versão que elimina divisões e reduz operações totais pode terminar antes mesmo com uma taxa menor. Veja a curva de tempo e a quantidade de trabalho registrada.

**Qual limitação precisa ser explicitada?** Newton pode divergir para certas estimativas iniciais ou encontrar uma Jacobiana singular. O caminho pivotado trata trocas e evita divisão por pivô final inutilizável. Ele conserva o passo de Newton integral. Para a análise experimental, contadores de hardware e estabilidade de frequência dependem da máquina utilizada.
