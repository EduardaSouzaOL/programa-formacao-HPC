# Teste inicial: IQ-TREE 2 com OpenMP

**Data:** 14/10/2026
**Máquina:** notebook com AMD Ryzen 5 PRO 4650U (6 núcleos físicos, 12 lógicos), executado no WSL2 com 3 GB de RAM visíveis
**Sistema:** Ubuntu no WSL2, arquivos no sistema de arquivos Linux (`~/`)
**Versão:** IQ-TREE 2.4.0 (multicore), compilado com GCC, com OpenMP ativo (`OpenMP : Yes` no CMake)
**Commit do código:** `a00094e0` (branch `master`)
**Compilação:** `cmake -DCMAKE_BUILD_TYPE=Release ..` seguido de `make -j6`, sem flags adicionais
**Dados:** `example/example.phy`, incluído no repositório (17 táxons, 1998 sítios, 1152 padrões distintos, DNA)

## Descrição do teste

O IQ-TREE recebe um alinhamento de sequências de DNA e procura a árvore filogenética que melhor explica esses dados pelo critério de máxima verossimilhança. Para isso, avalia muitas árvores candidatas e, para cada uma, calcula a probabilidade dos dados observados dado o modelo evolutivo. A busca termina quando não encontra árvore com verossimilhança melhor.

O modelo `GTR+G` está fixado, então o programa não gasta tempo escolhendo o modelo. A semente `-seed 1` também está fixada, para que as duas execuções sigam o mesmo caminho de busca.

## Comandos

```bash
./iqtree2 -s ../example/example.phy -m GTR+G -T 1 -seed 1 --prefix T1_rep1
./iqtree2 -s ../example/example.phy -m GTR+G -T 2 -seed 1 --prefix teste_inicial
```

Os parâmetros fixos são o modelo (`-m GTR+G`) e a semente (`-seed 1`). Assim, a única variável entre as execuções é o número de threads.

## O que é comparado

1. **Tempo total de parede** (`Total wall-clock time used`): o tempo que você percebe no relógio, do início ao fim da execução. É a principal métrica de desempenho.
2. **Tempo de parede da busca** (`Wall-clock time used for tree search`): só a etapa de busca da árvore, sem a leitura do arquivo e as etapas iniciais.
3. **Log-verossimilhança final** (`BEST SCORE FOUND`): verifica se o resultado é o mesmo. Se o paralelismo mudasse a resposta, o speedup não significaria nada.

## Como é calculado

- **Speedup:** S(p) = T(1) / T(p). Mostra quantas vezes a execução com p threads é mais rápida que a sequencial. Com 2 threads, o ideal seria 2.
- **Eficiência:** E(p) = S(p) / p. Mostra a fração do ideal que foi aproveitada.

## Resultados (1 execução por configuração)

| Threads | Tempo total (s) | Busca de árvore (s) | Speedup | Eficiência |
|---------|-----------------|---------------------|---------|------------|
| 1       | 5,021           | 4,776               | 1,00    | 100%       |
| 2       | 3,807           | 3,643               | 1,32    | 66%        |

**Verificação de equivalência:** a log-verossimilhança final foi `-21155.975` nas duas execuções (`BEST SCORE FOUND`). Os resultados são idênticos, o que indica que a paralelização não alterou a resposta neste exemplo.

## O que o teste não mostra

- **Não é uma medição de desempenho confiável.** O tempo total é de poucos segundos, e o ruído do sistema pode ser da mesma ordem da diferença medida.
- **Não compara as árvores diretamente.** A igualdade da log-verossimilhança é um indicador forte, mas a topologia poderia diferir em empates. Para verificar, dá para comparar os arquivos `.treefile` das duas execuções.
- **Não separa as etapas.** O paralelismo do IQ-TREE atua principalmente no cálculo da verossimilhança, sobre os padrões de sítios. A leitura e a parsimônia inicial não escalam, e isso aparece no speedup abaixo do ideal.

## Observações

- **Teste de validação, não de desempenho.** O alinhamento é pequeno e o tempo total é de poucos segundos. Por isso não se deve tirar conclusões de desempenho deste teste.
- **Uma execução por configuração.** O plano para os experimentos finais prevê 5 repetições por configuração, com mediana e dispersão.
- **Parte sequencial.** A leitura do alinhamento, o cálculo de distâncias e a parsimônia inicial não escalam com as threads. Isso limita o speedup, sobretudo em execuções curtas.
- **Limite de memória.** O IQ-TREE reporta 3 GB de RAM no cabeçalho. Esse é o limite que define o tamanho máximo do alinhamento nos experimentos.
- **Avisos de compilação.** O GCC emitiu avisos `-Wstringop-overflow` em código do módulo `cmaple`. Eles não impediram a geração do binário e não foram corrigidos neste teste.

