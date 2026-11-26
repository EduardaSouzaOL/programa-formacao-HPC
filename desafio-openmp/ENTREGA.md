# Desafio OpenMP: histograma de letras — implementação

Autora: Maria Eduarda S Oliveira (Duda) · CESAR School · Formação HPC 2026/2027
Data: 2026-11-26 · Plano de execução: ver `plano-desafio-openmp.md`.

## Como usar

```bash
make                    # compila omp-letters (serial) e gerar_texto
make entradas           # gera entradas/{pequena,media,grande}.txt e confere SHA-256
make versoes            # compila as 8 versões paralelas de uma vez

./confere.sh            # valida todas as versões contra a serial (média, 5 repetições por p)
./mede.sh omp-letters omp-letters-reduction ...   # mede tempos (grande, 5 repetições)
python3 analise.py resultados.csv [outro.csv]     # tabela + gráfico
./mede_schedule.sh      # laboratório da pergunta 3 (schedule)
```

## Versões implementadas

Cada arquivo altera **apenas `make_hist()`** em relação a `omp-letters.c`. O
cabeçalho GPLv3 e `COPYING` foram preservados; cada derivado registra sua
modificação no comentário do início.

| Arquivo | Estratégia | Observações |
| --- | --- | --- |
| `omp-letters-ingenua.c` | `#pragma omp parallel for` sem proteção | Serve para observar a condição de corrida; **errada**. Com `p=1` acerta (não há corrida). |
| `omp-letters-critical.c` | `#pragma omp critical` em volta das atualizações | Uma entrada na zona protegida por letra. |
| `omp-letters-atomic.c` | `#pragma omp atomic` em cada atualização | Uma `atomic` para `nlet`, outra para `hist[idx]`. |
| `omp-letters-local.c` | Histograma local por thread + `critical` por thread | Uma entrada na zona protegida por thread, não por letra. |
| `omp-letters-reduction.c` | `reduction(+:nlet)` + `reduction(+:hist[:ALPHA_LEN])` | O OpenMP faz a combinação. Exige OpenMP 4.5 ou mais novo. |
| `omp-letters-matriz.c` | **Minha versão**: matriz `h[nmax][26]` sem padding | `PAD=0`. Linhas de threads vizinhas dividem linhas de cache → **false sharing**. |
| `omp-letters-matriz-pad.c` | **Minha versão**: matriz `h[nmax][32]` com padding | `PAD=6`. Cada linha ocupa 128 bytes, 2 linhas de cache inteiras. |
| `omp-letters-sched.c` | `schedule(runtime)` com reduction | Laboratório para `OMP_SCHEDULE` (pergunta 3). |

**Por que escolhi a matriz com e sem padding.** Ela não está nos slides e mostra um fenômeno
que os slides só citam (*false sharing*). Comparar as duas variantes contra a `reduction` e a
`local` torna o papel do padding visível: a correção não muda, o tempo sim.

O alinhamento a 64 bytes é feito à mão (`calloc` + aritmética sobre `uintptr_t`), para
continuar em `-std=c99` e compilar com os mesmos flags do Makefile.

## Contêiner

- `hist.def` — receita Apptainer/Singularity (recomendado). Parte de `ubuntu:24.04` (GCC 13)
  e instala `gcc`, `make`, `libc6-dev` e `libgomp1`. Tem um `%test` que confere GCC + OpenMP.
- `Dockerfile` — alternativa para quem usa Docker (macOS, Windows ou Linux).

```bash
apptainer build hist.sif hist.def
apptainer exec hist.sif make
apptainer exec hist.sif ./confere.sh
RUN="apptainer exec hist.sif" SAIDA=resultados-container.csv ./mede.sh \
    omp-letters omp-letters-ingenua omp-letters-critical omp-letters-atomic \
    omp-letters-local omp-letters-reduction omp-letters-matriz omp-letters-matriz-pad
```

Como medi aqui (Docker Desktop no Windows; no PowerShell, troque `$PWD` pelo caminho da pasta):

```bash
docker build -t hist .
docker run --rm -v "$PWD":/work -w /work hist bash -c "make all versoes && make entradas && ./confere.sh"
docker run --rm -e SAIDA=resultados-container.csv -v "$PWD":/work -w /work hist \
    ./mede.sh omp-letters omp-letters-ingenua omp-letters-critical omp-letters-atomic \
    omp-letters-local omp-letters-reduction omp-letters-matriz omp-letters-matriz-pad
SAIDA=resultados-nativo.csv ./mede.sh omp-letters ...      # no WSL2, mesmos binários
python3 analise.py resultados-container.csv resultados-nativo.csv
```

## Ambiente da medição

| Item | Valor |
| --- | --- |
| CPU | AMD Ryzen 5 PRO 4650U (Zen 2, notebook) |
| Núcleos | 6 físicos, 12 lógicos (SMT 2 por núcleo); todos do mesmo tipo (sem P/E-cores) |
| Cache | L1d 32 KiB e L2 512 KiB por núcleo; L3 4 MiB compartilhado |
| Sistema | Windows 11 + WSL2 (Ubuntu 26.04) |
| Contêiner | Docker Desktop (backend WSL2), imagem `hist` do `Dockerfile` (Ubuntu 24.04), 12 CPUs, 3,7 GB |
| Compilador | GCC 13.3.0 (dentro do contêiner) |
| Flags | `-std=c99 -Wall -Wextra -Wpedantic -O2 -fopenmp` (Makefile) |
| Entrada | `entradas/grande.txt` (256 MiB, SHA-256 conferido) |
| Repetições | 5 por (versão, threads); uso a mediana |

Os mesmos binários (compilados no contêiner) foram medidos também direto no WSL2, para
que a única diferença entre as duas medições fosse o contêiner. Dados brutos:
`resultados-container.csv` e `resultados-nativo.csv`. Tabela: `tabela.csv`. Gráfico: `speedup.png`.

## Validação

`confere.sh` na entrada média, 5 repetições por número de threads, dentro do contêiner:
todas as versões dão **5/5** com p = 1, 2, 4, 8 e 12. A ingênua dá 5/5 com p=1 e **0/5**
com p ≥ 2; numa das execuções, o `main()` acusou
`ERRO: make_hist() devolveu 14292675 letras, mas o histograma soma 51394536`.

## Resultados (contêiner, entrada grande)

T_serial (`omp-letters`, mediana) = **1,355 s** (≈ 189 MiB/s).

Tempo de `make_hist()` em segundos (mediana de 5), speedup S = T_serial / T(p) e eficiência E = S / p:

| versão | p=1 | p=2 | p=4 | p=8 | p=12 |
| --- | --- | --- | --- | --- | --- |
| reduction | 1,394 | 0,497 | 0,412 | **0,291** | 0,306 |
| local | 0,936 | 0,771 | 0,456 | 0,314 | 0,343 |
| matriz-pad (minha) | 1,477 | 0,780 | 0,479 | 0,341 | 0,317 |
| matriz (minha) | 1,441 | 0,918 | 0,561 | 0,425 | 0,415 |
| ingênua (errada) | 1,440 | 1,388 | 1,430 | 2,099 | 1,774 |
| atomic | 2,184 | 3,819 | 5,180 | 7,698 | 6,059 |
| critical | 2,436 | 5,968 | 16,560 | 35,373 | 41,162 |

| versão | S(2) | S(4) | S(8) | S(12) | E(8) | E(12) |
| --- | --- | --- | --- | --- | --- | --- |
| reduction | 2,72 | 3,28 | **4,65** | 4,42 | 0,58 | 0,37 |
| local | 1,76 | 2,97 | 4,31 | 3,94 | 0,54 | 0,33 |
| matriz-pad | 1,74 | 2,83 | 3,97 | 4,28 | 0,50 | 0,36 |
| matriz | 1,48 | 2,42 | 3,19 | 3,27 | 0,40 | 0,27 |
| atomic | 0,35 | 0,26 | 0,18 | 0,22 | 0,02 | 0,02 |
| critical | 0,23 | 0,08 | 0,04 | 0,03 | 0,005 | 0,003 |

Melhor caso: `reduction` com 8 threads, 0,291 s (≈ 880 MiB/s), 4,65x a serial.

**Dispersão.** É um notebook, e a frequência varia muito. Exemplos: `reduction` p=1 foi de
0,93 s a 1,51 s; `atomic` p=1, de 2,17 s a 3,67 s. Por isso uso a mediana, e diferenças
abaixo de ~10% não devem ser lidas como reais.

## Análise

**1. A ingênua é mais rápida ou mais lenta que a serial?** Mais lenta, ou no máximo igual,
além de errada. Com 2 e 4 threads fica no mesmo tempo da serial (1,39 a 1,43 s), e com 8 e
12 piora (2,1 e 1,8 s). Todas as threads leem e escrevem `nlet` e `hist[]`, que estão na
mesma linha de cache. Cada escrita invalida essa linha nos outros núcleos, e a linha fica
pulando de um núcleo para outro (*cache ping-pong*). O trabalho divide, mas a memória
compartilhada serializa, e ainda se perdem atualizações (a corrida).

**2. critical e atomic: e o tempo?** As duas corrigem o resultado, mas são mais lentas que
a serial já com 1 thread (2,4 s e 2,2 s contra 1,35 s): cada letra paga o custo de entrar na
zona crítica ou de uma instrução atômica. Com mais threads piora: a `critical` chega a
41 s com 12 threads (30x mais lenta), porque uma trava global serializa tudo e as threads
ficam disputando a trava. A `atomic` é bem menos ruim (5 a 8 s), porque cada atualização é
uma instrução de hardware em um único endereço, sem trava. Mesmo assim, toda letra faz
`nlet` e uma posição de `hist` trocarem de núcleo. Conclusão: proteger cada atualização
corrige, mas a sincronização custa mais que a conta, que é muito barata.

**3. schedule.** Com `omp-letters-sched` e `OMP_SCHEDULE` (entrada média, p=12, mediana de 3):

| OMP_SCHEDULE | tempo |
| --- | --- |
| static | 0,085 s |
| static,1 | 0,285 s |
| static,4096 | 0,088 s |
| dynamic,1 | **1,264 s** |
| dynamic,64 | 0,107 s |
| dynamic,4096 | 0,096 s |
| dynamic,65536 | 0,078 s |
| guided / guided,64 | 0,097 / 0,098 s |

`dynamic,1` é ~15x mais lento que `static`: cada caractere vira uma negociação com o
runtime (um contador compartilhado). Com blocos maiores, o custo some: a partir de 4096,
`dynamic` empata com `static`. `static,1` também piora (3,4x), mas por outro motivo:
threads vizinhas processam bytes vizinhos e leem as mesmas linhas de cache, perdendo
localidade. Como o trabalho por caractere é uniforme, `static` (padrão) é a melhor escolha.

**4. Até quantas threads o speedup cresce?** Cresce até 8 e depois estabiliza: a `reduction`
vai de 4,65x (p=8) para 4,42x (p=12), e a `local`, de 4,31x para 3,94x. Hipóteses:

- *SMT*: só existem 6 núcleos físicos, e as threads 7 a 12 dividem núcleo com outra.
- *Frequência*: num notebook de 15 W, o turbo com 1 núcleo ativo é maior que com 6
  (então a eficiência cai mesmo antes de 6 threads).
- *Custo de coordenação*: criar o time e combinar 12 histogramas é fixo e pesa mais
  quando o laço fica curto (≈ 0,3 s).
- *Memória*: pouco provável. 880 MiB/s está muito abaixo da banda de memória (dezenas de
  GB/s), então o laço é limitado por CPU (`isalpha`/`tolower` e desvios), não por memória.

*Teste da hipótese do SMT* (`reduction`, entrada grande, mediana de 5):

| configuração | tempo |
| --- | --- |
| 6 threads, `OMP_PLACES=cores`, `OMP_PROC_BIND=close` (1 por núcleo) | 0,360 s |
| 6 threads, `OMP_PLACES=threads`, `OMP_PROC_BIND=close` | 0,338 s |
| 6 threads, sem afinidade | 0,329 s |
| 12 threads, sem afinidade | 0,305 s |

Passar de 6 para 12 threads ganha só ~8%: o SMT ajuda pouco porque as duas threads
de um núcleo disputam as mesmas unidades de execução. Mas 6 threads, uma por núcleo,
dão só ~3,8x, e não 6x. Então o SMT explica por que o speedup não cresce depois de 6
threads, mas não explica a eficiência abaixo de 1 antes disso. Para essa parte, a
hipótese mais forte é a queda de frequência com vários núcleos ativos, que não consegui
medir de dentro do WSL/contêiner.

**5. Minha versão contra a reduction.** A `matriz-pad` fica perto da `reduction` e empata
com a `local`. Com 12 threads: 0,317 s, contra 0,306 s da `reduction` e 0,343 s da `local`;
com 8: 0,341 s, contra 0,291 s e 0,314 s. Isso
faz sentido: as três fazem a mesma coisa, um histograma por thread e uma combinação no fim.
A `reduction` ganha um pouco porque o compilador sabe que a cópia é privada e pode mantê-la
em registradores ou na pilha. Na matriz, a linha de cada thread mora num array compartilhado,
acessado por ponteiro. O resultado mais interessante é o **padding**: sem ele, a `matriz`
fica de 17% a 31% mais lenta com 2 a 12 threads (0,425 s contra 0,341 s com p=8;
0,415 s contra 0,317 s com p=12). Com 1 thread, as duas empatam (1,44 s e 1,48 s), como
esperado: sem outra thread, não há com quem dividir a linha de cache. Sem
padding, cada linha tem 104 bytes, e as linhas de threads vizinhas dividem uma linha de
cache de 64 bytes: *false sharing*. Com `PAD=6`, cada linha ocupa 128 bytes alinhados, e
o efeito some. O resultado é o mesmo; só o tempo muda.

**T_serial contra T(1).** `reduction` com 1 thread: 1,394 s contra 1,355 s da serial (≈ 3%,
dentro do ruído), então o custo do OpenMP com uma thread é pequeno. Na `local` com 1 thread
(0,936 s), o tempo fica *abaixo* da serial. Minha hipótese: com o histograma em um array
local, o compilador sabe que `meu[]` não aponta para o mesmo lugar que `text`. Na serial,
`hist` é um ponteiro que poderia apontar para o texto (*aliasing*), e isso impede algumas
otimizações. Pelo mesmo motivo, a `reduction` com p=2 tem eficiência acima de 1 (1,36).

**6. Tipos de núcleo.** O Ryzen 5 PRO 4650U tem 6 núcleos iguais (sem P/E-cores), então
esse efeito não aparece aqui.

**CPU contra parede.** `time` da `reduction` no contêiner: com p=1, `user` 1,51 s; com
p=8, `user` 2,18 s para 0,29 s de laço. O tempo de CPU somado cresce com as threads (espera
na barreira, SMT, combinação), mesmo quando a parede cai. O `real` foi ~5 s nos dois casos,
quase todo para ler 256 MiB da pasta do Windows montada no contêiner. Por isso o tempo
medido é só o de `make_hist()`.

## Contêiner contra nativo

Mesmos binários, mediana de 5, entrada grande (razão = WSL2 / contêiner):

| versão | p | contêiner | WSL2 | razão |
| --- | --- | --- | --- | --- |
| serial | 1 | 1,355 | 1,332 | 0,98 |
| reduction | 8 | 0,291 | 0,271 | 0,93 |
| reduction | 12 | 0,306 | 0,278 | 0,91 |
| local | 12 | 0,343 | 0,301 | 0,88 |
| matriz-pad | 12 | 0,317 | 0,305 | 0,96 |
| critical | 8 | 35,37 | 70,05 | 1,98 |
| critical | 12 | 41,16 | 70,50 | 1,71 |

Para as versões que escalam, a diferença fica em torno de 10%, na faixa do ruído. O
contêiner não acrescenta custo perceptível ao laço: ele é só um processo Linux isolado
por *namespaces* e *cgroups*, rodando no mesmo kernel e nas mesmas CPUs, sem emulação.
A exceção é a `critical`, cerca de 2x mais lenta no WSL2. Ela é dominada pela disputa da
trava, que é muito sensível ao escalonador e a onde as threads caem (o Docker Desktop
roda numa VM WSL2 separada, com outra carga e outra configuração). Cuidado: no Windows,
o Docker Desktop também roda sobre o WSL2, então "nativo" aqui é o WSL2, não o Windows
direto. O custo real do contêiner apareceu fora do laço, na leitura da pasta montada (`real`
≈ 5 s).
