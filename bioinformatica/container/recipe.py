"""
Receita HPCCM do IQ-TREE 2.4.0 (OpenMP), equivalente ao container/Dockerfile.

Gerar os arquivos:
    hpccm --recipe recipe.py --format docker      > Dockerfile.hpccm
    hpccm --recipe recipe.py --format singularity --singularity-version=3.2 > iqtree2.hpccm.def

Versões fixadas (podem ser trocadas com --userarg):
    hpccm --recipe recipe.py --format docker --userarg iqtree_ref=a00094e0
"""

# Argumentos opcionais (userarg) com os valores fixados por padrão
base_image = USERARG.get('base_image', 'ubuntu:22.04')
iqtree_ref = USERARG.get('iqtree_ref', 'a00094e0')

# ---------- Estágio 0: compilação ----------
Stage0 += baseimage(image=base_image, _as='build')

Stage0 += apt_get(
    ospackages=['build-essential', 'cmake', 'git', 'ca-certificates',
                'libeigen3-dev', 'libboost-dev', 'zlib1g-dev'])

Stage0 += shell(commands=[
    'mkdir -p /src',
    'cd /src',
    'git clone --recursive https://github.com/iqtree/iqtree2.git .',
    'git checkout {}'.format(iqtree_ref),
    'git submodule update --init --recursive',
    'mkdir build',
    'cd build',
    'cmake -DCMAKE_BUILD_TYPE=Release ..',
    'make -j2',
])

# ---------- Estágio 1: imagem final ----------
# _as='final' vira "Stage: final" no .def (o HPCCM não nomeia o 2º estágio sozinho)
Stage1 += baseimage(image=base_image, _as='final')

Stage1 += apt_get(ospackages=['libgomp1', 'zlib1g'])

Stage1 += copy(_from='build',
                src='/src/build/iqtree2',
                dest='/usr/local/bin/iqtree2')

Stage1 += workdir(directory='/data')

# ENTRYPOINT no Docker e %runscript no Apptainer, como no Dockerfile e no iqtree2.def
Stage1 += runscript(commands=['iqtree2'])
