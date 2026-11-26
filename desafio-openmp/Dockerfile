# Alternativa ao hist.def, para quem usa Docker (macOS, Windows ou Linux).
# Uso (do diretório do projeto):
#   docker build -t hist .
#   docker run --rm -u "$(id -u):$(id -g)" -v "$PWD":/work -w /work hist make
#   docker run --rm -i -e OMP_NUM_THREADS=8 -v "$PWD":/work -w /work hist \
#       ./omp-letters-reduction < entradas/grande.txt
FROM ubuntu:24.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        gcc make libc6-dev libgomp1 && \
    rm -rf /var/lib/apt/lists/*

ENV LC_ALL=C
WORKDIR /work
