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

(preencher)

## Resultados

(preencher com `tabela.csv`, `speedup.png` e `fases.png`)

## Análise

(preencher)
