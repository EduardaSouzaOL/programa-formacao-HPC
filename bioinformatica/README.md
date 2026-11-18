# Caracterização de desempenho do IQ-TREE 2 com OpenMP

Estudo de desempenho do [IQ-TREE 2](http://www.iqtree.org/), um software de inferência filogenética por máxima verossimilhança. O trabalho faz parte do Programa de Formação em Computação de Alto Desempenho (CESAR, 2026/2027), no desafio de escolher um software open source de uma área de aplicação real, documentar sua compilação e execução e containerizá-lo para rodar no cluster com Apptainer.

## Contexto

A filogenética estuda a relação evolutiva entre espécies ou sequências. Dado um alinhamento de sequências, o objetivo é encontrar a árvore que melhor explica os dados. O IQ-TREE faz essa busca por máxima verossimilhança, o que exige avaliar muitas árvores candidatas e calcular a verossimilhança de cada uma. Esse cálculo é caro e pode ser paralelizado, que é o foco deste estudo.

## Versão

- IQ-TREE multicore **2.4.0** para Linux x86 64-bit
- Commit de referência do código: `a00094e0` (branch `master` do IQ-TREE 2)
- Build nativa de 14/10/2026 e build em contêiner de 18/11/2026, ambas verificadas com `iqtree2 --version`

## Ambiente

- **Máquina:** notebook com AMD Ryzen 5 PRO 4650U (6 núcleos físicos, 12 lógicos)
- **Sistema:** Windows 11 com WSL2 (Ubuntu); o WSL enxerga 8 núcleos
- **Memória disponível para o WSL:** 3 GB
- **Compilador:** GCC
- **Paralelismo:** OpenMP (ativo por padrão no CMake do IQ-TREE)
- **Contêineres:** Docker (desenvolvimento) e Apptainer (cluster)

Os registros completos do ambiente ficam em `docs/`.

## Estrutura

```
bioinformatica/
├── container/
│   ├── Dockerfile            # imagem do IQ-TREE 2.4.0 (dois estágios)
│   ├── iqtree2.def           # definition file do Apptainer (escrito à mão)
│   ├── recipe.py             # receita HPCCM
│   ├── Dockerfile.hpccm      # Dockerfile gerado pelo HPCCM
│   └── iqtree2.hpccm.def     # definition file gerado pelo HPCCM
├── scripts/
│   └── bench.sh              # repete execuções e calcula média e desvio
├── docs/                     # documentação dos testes e do procedimento
│   └── teste-inicial.md
├── data/                     # alinhamentos e resultados (sem versionamento)
├── third_party/
│   └── iqtree2/              # código-fonte do IQ-TREE 2
├── .dockerignore
├── .gitignore
└── README.md
```

O `Dockerfile` foi construído e executado. O `iqtree2.def`, a receita HPCCM e os arquivos gerados por ela ainda não foram construídos com Apptainer.

## Como reproduzir

### Compilação nativa

1. Clone o IQ-TREE 2 com submódulos e fixe o commit:
   ```bash
   git clone --recursive https://github.com/iqtree/iqtree2.git third_party/iqtree2
   cd third_party/iqtree2
   git checkout a00094e0
   git submodule update --init --recursive
   ```

2. Instale as dependências (CMake, GCC, Eigen3, Boost, zlib).

3. Compile:
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j2
   ```
   Com 3 GB de memória no WSL, evite `-j` alto: a compilação pode ser encerrada por falta de memória.

4. Confirme a versão:
   ```bash
   ./iqtree2 --version
   ```

Para o procedimento completo do teste inicial, veja `docs/teste-inicial.md`.

### Contêiner (Docker)

A imagem usa dois estágios: o primeiro compila o IQ-TREE no commit fixado, e o segundo copia apenas o binário e as bibliotecas de execução. Versões fixadas: base `ubuntu:22.04` e commit `a00094e0`.

1. Construa a imagem a partir da pasta `bioinformatica/`:
   ```bash
   docker build -t iqtree2:2.4.0 -f container/Dockerfile .
   ```

2. Confirme a versão:
   ```bash
   docker run --rm iqtree2:2.4.0 --version
   ```

3. Baixe o alinhamento de exemplo e execute o teste. Os dados ficam fora da imagem e entram por volume:
   ```bash
   mkdir -p data
   curl -L -o data/example.phy https://raw.githubusercontent.com/iqtree/iqtree2/master/example/example.phy

   docker run --rm -v "$PWD/data":/data iqtree2:2.4.0 \
       -s example.phy -m GTR+G -T 2 --seed 1 --prefix teste_inicial
   ```

4. Veja o tamanho da imagem:
   ```bash
   docker images iqtree2
   ```

### Contêiner (Apptainer)

O `container/iqtree2.def` tem os mesmos dois estágios e as mesmas versões do Dockerfile. Requer Apptainer com suporte a builds multiestágio.

```bash
apptainer build iqtree2.sif container/iqtree2.def

apptainer run --pwd /data -B "$PWD/data":/data iqtree2.sif \
    -s example.phy -m GTR+G -T 2 --seed 1 --prefix apptainer -redo
```

O `--pwd /data` é necessário: ao contrário do Docker (`WORKDIR /data`), o Apptainer mantém por padrão o diretório atual do host como diretório de trabalho, e o IQ-TREE não encontraria `example.phy`. Alternativa sem `--pwd`: entrar em `data/` e rodar `apptainer run ../iqtree2.sif -s example.phy ...`.

O `.sif` não é versionado (está no `.gitignore`); quem clonar o repositório o reconstrói com o comando acima.

### Receita HPCCM

A receita `container/recipe.py` descreve a mesma imagem e gera o Dockerfile e o definition file:

```bash
pip install hpccm
cd container
hpccm --recipe recipe.py --format docker > Dockerfile.hpccm
hpccm --recipe recipe.py --format singularity --singularity-version=3.2 > iqtree2.hpccm.def
```

A base e o commit podem ser trocados sem editar a receita, por exemplo `--userarg iqtree_ref=a00094e0`.

### Medição de desempenho

O `scripts/bench.sh` repete uma execução N vezes e imprime uma linha CSV por rodada (log-likelihood, tempo de parede e tempo de CPU reportados pelo IQ-TREE, mais o tempo externo do comando inteiro), e no fim a média e o desvio-padrão. O tempo externo inclui a partida do Docker/Apptainer, que o IQ-TREE não enxerga. O comando medido deve incluir `-redo`.

```bash
chmod +x scripts/bench.sh

scripts/bench.sh nativo 10 third_party/iqtree2/build/iqtree2 \
    -s data/example.phy -m GTR+G -T 2 --seed 1 --prefix data/nativo -redo

scripts/bench.sh docker 10 docker run --rm --user "$(id -u):$(id -g)" \
    -v "$PWD/data":/data iqtree2:2.4.0 \
    -s example.phy -m GTR+G -T 2 --seed 1 --prefix docker -redo

scripts/bench.sh apptainer 10 apptainer run --pwd /data -B "$PWD/data":/data iqtree2.sif \
    -s example.phy -m GTR+G -T 2 --seed 1 --prefix apptainer -redo
```

### Resultado de referência

Execução do teste no contêiner Docker (`-T 2`, `--seed 1`, 17 sequências, 1998 colunas):

| Item | Valor |
|---|---|
| Melhor log-likelihood | -21155.975 |
| Comprimento total da árvore | 4.198 |
| Tempo de parede total | 3.735 s |
| Tempo de CPU total | 7.166 s |

Uma execução nativa ou em outra imagem, com a mesma versão, o mesmo `-T` e a mesma `--seed`, deve chegar ao mesmo log-likelihood e à mesma árvore (`.treefile`). Esse é o critério de reprodutibilidade usado neste trabalho.

## Próximos passos

- Construir e testar o `iqtree2.def` e os arquivos gerados pelo HPCCM com Apptainer
- Medir nativo, Docker e Apptainer com o `scripts/bench.sh` e registrar média e desvio
- Registrar o tamanho final da imagem e do `.sif`, as otimizações feitas e as dificuldades encontradas
