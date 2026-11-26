# Alternativa ao mpi.def, para quem usa Docker (macOS, Windows ou Linux).
# O mpirun recusa rodar como root; as duas variáveis abaixo liberam isso dentro
# do contêiner. Com -u "$(id -u):$(id -g)" elas não são necessárias.
# Uso (do diretório do projeto):
#   docker build -t mpi .
#   docker run --rm -v "$PWD":/work -w /work mpi make all versoes
#   docker run --rm -i -v "$PWD":/work -w /work mpi \
#       mpirun -np 4 ./mpi-letters-coletivas < entradas/grande.txt
FROM ubuntu:24.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        gcc make libc6-dev libgomp1 openmpi-bin libopenmpi-dev && \
    rm -rf /var/lib/apt/lists/*

ENV LC_ALL=C \
    HWLOC_COMPONENTS=-gl \
    OMPI_ALLOW_RUN_AS_ROOT=1 \
    OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1
WORKDIR /work
