# Desafio MPI: histograma de letras, implementação

Autora: Maria Eduarda S Oliveira (Duda) · CESAR School · Formação HPC 2026/2027
Data: 2026-11-26

## Como usar

```bash
make                    # compila letters (serial) e gerar_texto
make entradas           # gera entradas/{pequena,media,grande}.txt e confere SHA-256
make versoes            # compila as 3 versões MPI

./confere.sh                                   # valida as versões contra a serial
./mede.sh letters mpi-letters-coletivas mpi-letters-p2p mpi-letters-arquivo
python3 analise.py resultados.csv ../desafio-openmp/resultados-container.csv
```

`mpi-letters-coletivas` e `mpi-letters-p2p` leem a entrada padrão. `mpi-letters-arquivo`
recebe o nome do arquivo por argumento: `mpirun -np 4 ./mpi-letters-arquivo entradas/grande.txt`.

Todas escrevem o histograma na saída padrão e uma linha de tempos na saída de erro, com o
maior tempo entre os processos em cada fase (`MPI_Reduce` com `MPI_MAX`):

```
distribuir 0.037140 contar 0.076838 combinar 0.019545 total 0.100451
```

Na versão `arquivo`, a primeira fase se chama `ler`.

## Versões implementadas

Cada arquivo parte de `letters.c` e muda só o `main()`. `make_hist()`, `print_hist()` e
`read_all()` são os mesmos da serial. O cabeçalho GPLv3 e `COPYING` foram preservados.

| Arquivo | Distribuir | Combinar |
| --- | --- | --- |
| `mpi-letters-coletivas.c` | `MPI_Bcast` do tamanho + `MPI_Scatterv` | `MPI_Reduce` |
| `mpi-letters-p2p.c` | `MPI_Send`/`MPI_Recv` do tamanho e dos pedaços, um rank por vez | `MPI_Recv` com `MPI_ANY_SOURCE` e soma no rank 0 |
| `mpi-letters-arquivo.c` (minha) | cada rank lê o seu pedaço do arquivo com `fseeko` | `MPI_Reduce` |

A divisão é a mesma nas três: `meu_n = n / p + (rank < n % p)`, então o resto vai para os
primeiros ranks, uma letra para cada, e nenhum byte fica de fora.

Detalhes:

- Os `count` e deslocamentos do `Scatterv` e do `Send` são `int`. O rank 0 confere se a
  entrada cabe em `INT_MAX` (a grande tem 268.435.456 bytes) e aborta se não couber.
- `malloc(meu_n > 0 ? meu_n : 1)`: com o arquivo vazio, `malloc(0)` pode devolver `NULL`.
- Na p2p, o pedaço do rank 0 não viaja (ele conta direto em `text`), e o total de letras
  vai na mesma mensagem do histograma, na posição 26. Tags diferentes para tamanho, pedaço
  e histograma. Não há risco de deadlock: o rank 0 só manda e depois só recebe, e os
  outros fazem o contrário.
- Na `arquivo`, o tamanho vem de `ftello` no rank 0 e vai por `MPI_Bcast` como
  `long long`/`MPI_LONG_LONG`, então não há limite de 2 GB.

**Por que escolhi ler direto do arquivo.** Nas outras duas versões, o rank 0 lê o texto
inteiro e manda cada pedaço: essa é a parte serial que não encolhe com p. Na minha versão,
cada processo lê só o que vai contar, e nenhum guarda o texto inteiro, que é o que seria
preciso com o texto espalhado em várias máquinas. As outras ideias (Allreduce, Gather,
árvore) mexem só na fase de combinar, que soma 26 inteiros e já é pequena.

Comparação justa: nas versões `coletivas` e `p2p`, a leitura (`read_all`) fica fora do
tempo, como pede o enunciado. Na `arquivo`, ler é o próprio jeito de distribuir, então
entra no tempo. Por isso ela já sai em desvantagem nessa comparação. A segunda leitura do
mesmo arquivo vem do cache do sistema, então a primeira repetição pode ser mais lenta.

## Contêiner

- `mpi.def`: receita Apptainer. É o `hist.def` do desafio OpenMP com `openmpi-bin` e
  `libopenmpi-dev` (Open MPI 4.1.6 no Ubuntu 24.04). Mantive `libgomp1` para compilar as
  versões OpenMP na mesma imagem. O `%test` confere o GCC, o `mpirun` e compila um
  programa MPI mínimo.
- `Dockerfile`: alternativa com Docker, com `OMPI_ALLOW_RUN_AS_ROOT` para o `mpirun`
  aceitar rodar como root dentro do contêiner.

```bash
docker build -t mpi .
docker run --rm -v "$PWD":/work -w /work mpi bash -c "make all versoes && make entradas && ./confere.sh"
docker run --rm -v "$PWD":/work -w /work mpi ./mede.sh letters mpi-letters-coletivas mpi-letters-p2p mpi-letters-arquivo
```

## Validação

`confere.sh` compara com `diff` a saída de cada versão com a da serial: entrada média com
p = 1, 2, 3, 4, 8 e `nproc` (3 não divide o texto), arquivo vazio com 4 processos, `abc`
com 4 e 8 processos (menos letras que processos) e a entrada pequena com 1 processo. Acima
do número de núcleos, usa `--oversubscribe`. As três versões passaram em todos os casos.

## Ambiente da medição

| Item | Valor |
| --- | --- |
| CPU | AMD Ryzen 5 PRO 4650U (Zen 2, notebook) |
| Núcleos | 6 físicos, 12 lógicos (SMT 2 por núcleo), um socket, um nó NUMA |
| Sistema | Windows 11 + WSL2 |
| Contêiner | Docker Desktop (backend WSL2), imagem `mpi` do `Dockerfile` (Ubuntu 24.04); o contêiner vê as 12 CPUs |
| Compilador | GCC 13.3.0 com `mpicc` (Open MPI 4.1.6), `-std=c99 -Wall -Wextra -Wpedantic -O2` |
| Entrada | `entradas/grande.txt` (256 MiB, SHA-256 conferido), na pasta do Windows montada no contêiner |
| Repetições | 5 por (versão, p); uso a mediana |

Dados brutos: `resultados-container.csv`. Tabela: `tabela.csv`. Gráficos: `speedup.png` e `fases.png`.

## Resultados (contêiner, entrada grande)

T_serial (`letters`, mediana de `make_hist()`) = **1,448 s** (≈ 177 MiB/s).

Mediana de cada fase e de T(p) em segundos, speedup S = T_serial / T(p) e eficiência E = S / p.
Em cada fase vale o processo mais lento, e os máximos podem vir de processos diferentes:
por isso a soma das fases pode passar de T(p).

| versão | p | distribuir | contar | combinar | T(p) | S | E |
| --- | --- | --- | --- | --- | --- | --- | --- |
| coletivas | 1 | 0,474 | 0,892 | 0,000 | 1,397 | 1,04 | 1,04 |
| coletivas | 2 | 0,528 | 0,471 | 0,000 | 1,009 | 1,44 | 0,72 |
| coletivas | 4 | 0,418 | 0,341 | 0,238 | 0,770 | 1,88 | 0,47 |
| coletivas | 6 | 0,754 | 0,247 | 0,480 | 0,976 | 1,48 | 0,25 |
| coletivas | 8 | 0,532 | 0,183 | 0,349 | 0,718 | 2,02 | 0,25 |
| p2p | 1 | 0,000 | 1,339 | 0,000 | 1,339 | 1,08 | 1,08 |
| p2p | 2 | 0,246 | 0,500 | 0,000 | 0,742 | 1,95 | 0,98 |
| p2p | 4 | 0,394 | 0,334 | 0,000 | 0,688 | 2,11 | 0,53 |
| p2p | 6 | 0,371 | 0,261 | 0,000 | 0,594 | 2,44 | 0,41 |
| p2p | 8 | 0,415 | 0,198 | 0,000 | **0,585** | 2,47 | 0,31 |
| arquivo (minha) | 1 | 2,598 | 1,280 | 0,000 | 3,877 | 0,37 | 0,37 |
| arquivo (minha) | 2 | 1,622 | 0,725 | 0,000 | 2,348 | 0,62 | 0,31 |
| arquivo (minha) | 4 | 1,508 | 0,389 | 0,033 | 1,879 | 0,77 | 0,19 |
| arquivo (minha) | 6 | 1,454 | 0,244 | 0,032 | 1,697 | 0,85 | 0,14 |
| arquivo (minha) | 8 | 1,122 | 0,191 | 0,017 | 1,311 | 1,10 | 0,14 |

Na versão `arquivo`, a coluna distribuir é a fase "ler". Com p = 8 passo de 6 núcleos
físicos, e o `mede.sh` acrescenta `--oversubscribe`.

**Comparação com o OpenMP** (mesma máquina, mesma entrada, `omp-letters-reduction`, a melhor
versão do desafio anterior, dados de `../desafio-openmp/resultados-container.csv`):

| p | coletivas | p2p | arquivo | OpenMP reduction |
| --- | --- | --- | --- | --- |
| 1 | 1,397 | 1,339 | 3,877 | 1,394 |
| 2 | 1,009 | 0,742 | 2,348 | 0,497 |
| 4 | 0,770 | 0,688 | 1,879 | 0,412 |
| 8 | 0,718 | 0,585 | 1,311 | **0,291** |

**Dispersão.** As repetições variam bastante neste notebook. A pior foi `coletivas` com p = 6:
de 0,64 s a 7,45 s, por causa de uma única execução lenta. Por isso a linha p = 6 das
coletivas parece pior que p = 4 e p = 8, e não leio essa diferença como real. Na `arquivo`
com p = 1, o tempo cresceu de 2,4 s a 4,6 s ao longo das repetições, então a leitura
da pasta montada não ficou mais rápida nas repetições seguintes, como seria de esperar com o cache.

## Análise

**1. Onde está o tempo com 1 e com 8 processos? Quando a comunicação domina?** Com 1
processo, a p2p é só contar (1,34 s), e a `coletivas` gasta 0,47 s em distribuir: o
`Scatterv` copia os 256 MiB do `text` para o buffer `meu` do próprio rank 0, e a p2p conta
direto em `text`. Com 8 processos, contar cai para 0,18 a 0,20 s (7x menos), mas distribuir
fica em 0,4 a 0,5 s: o rank 0 continua mandando 256 MiB, só que em mais pedaços. A
comunicação passa a dominar já em p = 2 nas `coletivas` (0,53 contra 0,47 s) e em p = 4 na
p2p (0,39 contra 0,33 s). É o que o slide 36 chama de Amdahl de novo: distribuir é a parte
serial e não encolhe com p.

O `combinar` alto das `coletivas` (0,24 a 0,48 s a partir de p = 4) não é custo de somar 26
inteiros. O `Scatterv` entrega os pedaços em sequência: o rank 1 recebe primeiro, conta e
fica esperando dentro do `MPI_Reduce` até o último rank terminar. Esse tempo de espera
aparece como combinar. Na p2p isso não acontece, porque o rank 0 só conta o próprio pedaço
depois de mandar todos os outros; quando ele chega no `MPI_Recv`, os histogramas já estão lá.

**2. MPI ou OpenMP?** OpenMP, por quase 2x: 0,291 s contra 0,585 s da melhor versão MPI com 8.
Mas o interessante é que só a fase contar do MPI com 8 processos (0,18 a 0,20 s) é mais
rápida que o `make_hist()` inteiro do OpenMP. A diferença está toda em mover o texto: as
threads do OpenMP leem o mesmo `text` na memória, e os processos MPI precisam receber uma
cópia. Para um problema que lê cada byte uma vez e faz uma conta barata, copiar os dados
custa quase o mesmo que processá-los.

**3. A p2p é mais lenta que a de coletivas?** Não: foi mais rápida em todos os p (0,585 contra
0,718 s com 8). Três motivos: o pedaço do rank 0 não é copiado; o Open MPI 4.1 usa por padrão
um algoritmo linear no `Scatterv` (o root manda um pedaço de cada vez, como o meu
laço de `MPI_Send`), então a coletiva não ganha uma árvore; e o `MPI_ANY_SOURCE` deixa o rank 0
somar os histogramas na ordem em que chegam. A coletiva é mais curta e mais difícil de errar,
mas aqui não é mais rápida.

**4. `--report-bindings`.** Com 2 processos, o Open MPI prende cada rank a um núcleo
(`[BB/../../../../..]` e `[../BB/../../../..]`). Com 4, nenhum rank é preso: a regra padrão
passa a ser prender ao socket, e como só há um socket, isso é o mesmo que todas as CPUs. Com
`--bind-to core`, os 4 ranks vão para os núcleos 0 a 3, com o aviso "failed to bind memory"
(o contêiner não pode fixar memória por NUMA, o que não importa com um nó NUMA só). Nos tempos,
não fez diferença: `coletivas` com 4 deu mediana de 0,899 s sem e 0,854 s com `--bind-to core`,
dentro do ruído.

**5. Mais processos que núcleos.** Dentro do contêiner, `mpirun -np 6` roda e `-np 7` falha
com "not enough slots": o padrão é um processo por núcleo físico. Com `--use-hwthread-cpus`,
o limite vira 12 e o 13º falha. Fora do contêiner, no WSL (que tem um Open MPI 5 instalado), o
limite é o mesmo: 6, e 12 com `--use-hwthread-cpus`. O contêiner enxerga as mesmas CPUs que o
WSL, então não muda nada. Nos tempos da p2p: 0,614 s com 6, 0,740 s com 12
(`--use-hwthread-cpus`) e 0,676 s com 16 (`--oversubscribe`). Contar ainda cai (0,23 s com 6;
0,16 com 12; 0,14 com 16), mas distribuir sobe (0,41 s para 0,53 a 0,59 s) e o total piora.
Passar dos núcleos físicos não ajuda.

**6. Se o texto estivesse em 4 máquinas.** O código de coletivas e p2p não serve: o rank 0
precisaria ter o texto inteiro na memória e mandar tudo pela rede, que é mais lenta que a
memória compartilhada daqui. A minha versão `arquivo` é o caminho: cada processo lê só a sua
parte, do disco local da máquina ou de um sistema de arquivos paralelo, e só os 26 contadores
viajam. Para 1 TB eu mudaria ainda: os contadores e o `nlet` para `long long`, com
`MPI_LONG_LONG` no `MPI_Reduce` (um `int` estoura em 2^31 letras); e a leitura em blocos de
alguns MiB num laço, contando cada bloco, porque um pedaço de 250 GB não cabe na memória.
O `mpirun` passaria a usar um `--hostfile` com as 4 máquinas.

**7. Minha versão contra a de coletivas.** Mais lenta neste notebook: 1,311 s contra 0,718 s
com 8. Quase todo o tempo é a fase ler (1,1 a 2,6 s), porque o arquivo está na pasta do
Windows montada no contêiner, e ler por essa montagem é lento: `cat entradas/grande.txt`
dentro do contêiner leva 2,0 s (≈ 128 MiB/s). Mesmo assim, ler em paralelo ajuda: a fase ler
cai de 2,6 s com 1 processo para 1,1 s com 8. A comparação também não é justa com ela: nas
outras duas, a leitura do rank 0 fica fora do T(p), como pede o enunciado, e custaria esses
mesmos ~2 s. Somando a leitura, a `coletivas` com 8 ficaria em torno de 2,7 s, e a `arquivo`,
em 1,3 s. Quando a leitura entra na conta, a minha versão é a mais rápida.

**Nativo contra contêiner.** Não medi o MPI fora do contêiner: o Open MPI do WSL é a versão 5,
diferente da 4.1.6 da imagem, e eu só tenho os binários compilados contra a 4.1.6.

Saída completa dos testes de afinidade, slots e oversubscribe: `testes-mpirun.txt`.
