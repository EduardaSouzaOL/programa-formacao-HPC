FROM ubuntu:22.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive
# commit fixado (o mesmo do README)
ARG IQTREE_REF=a00094e0

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git ca-certificates \
        libeigen3-dev libboost-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
RUN git clone --recursive https://github.com/iqtree/iqtree2.git . \
    && git checkout ${IQTREE_REF} \
    && git submodule update --init --recursive

RUN mkdir build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make -j2


FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        libgomp1 zlib1g \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/iqtree2 /usr/local/bin/iqtree2

WORKDIR /data
ENTRYPOINT ["iqtree2"]