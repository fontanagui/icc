# Padrão de escrita usado nos arquivos C

Referências de organização: códigos de Newton/Gauss fornecidos pelo professor e projeto The Boys (C).

- Nomes de funções curtos e descritivos, em português quando fizer sentido: `calcF`, `calcJ`, `newton`, `triang`, `retrossubs`.
- Espaço antes e depois do conteúdo dos parênteses: `if ( condicao )`, `for ( i = 0; i < n; i++ )`, `funcao( X, n )`.
- Chave de abertura na mesma linha da operação: `if ( condicao ) {`.
- Sem chaves em `if`/`for` com apenas uma instrução.
- Linha em branco separando `if` independentes.
- Linha em branco antes de `return`, exceto quando o `return` é a única instrução dentro de um `if`.
- Comentários curtos que expliquem o motivo de um trecho, não descrições longas do óbvio.
- Funções pequenas sempre que possível; nenhuma alocação de memória dentro do laço de Newton.

**Observação importante:** a semelhança de estilo com o exemplo do professor não muda o problema: este projeto calcula explicitamente a função e a Jacobiana de **Broyden** do enunciado. O exemplo genérico com `libmatheval` foi usado como referência de organização, não incorporado à implementação.
