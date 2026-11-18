#!/usr/bin/env bash
# Repete uma execução do IQ-TREE N vezes e resume log-likelihood e tempos.
#
# Uso:    scripts/bench.sh <rotulo> <N> <comando...>
# Saída:  linhas CSV (rotulo,rodada,loglik,parede_s,cpu_s) em stdout
#         e média/desvio-padrão em stderr.
#
# O comando deve incluir -redo, para o IQ-TREE não reaproveitar o checkpoint.
# Exemplos:
#   scripts/bench.sh nativo 10 third_party/iqtree2/build/iqtree2 \
#       -s data/example.phy -m GTR+G -T 2 --seed 1 --prefix data/nativo -redo
#   scripts/bench.sh docker 10 docker run --rm -v "$PWD/data":/data iqtree2:2.4.0 \
#       -s example.phy -m GTR+G -T 2 --seed 1 --prefix docker -redo
#   scripts/bench.sh apptainer 10 apptainer run -B "$PWD/data":/data iqtree2.sif \
#       -s example.phy -m GTR+G -T 2 --seed 1 --prefix apptainer -redo
set -euo pipefail

if [ "$#" -lt 3 ]; then
    sed -n '2,15p' "$0" >&2
    exit 1
fi

label=$1; n=$2; shift 2
tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT

for i in $(seq 1 "$n"); do
    "$@" > "$tmp" 2>&1 || { echo "Falha na rodada $i:" >&2; tail -n 5 "$tmp" >&2; exit 1; }
    lnl=$(grep -m1 "BEST SCORE FOUND" "$tmp" | awk '{print $5}')
    wall=$(grep -m1 "Total wall-clock time used" "$tmp" | awk '{print $5}')
    cpu=$(grep -m1 "Total CPU time used" "$tmp" | awk '{print $5}')
    echo "${label},${i},${lnl},${wall},${cpu}"
done | tee >(awk -F, -v label="$label" '
    { n++; w+=$4; w2+=$4*$4; c+=$5; c2+=$5*$5; l[$3]++ }
    END {
        mw=w/n; mc=c/n
        sw=(n>1)?sqrt((w2-n*mw*mw)/(n-1)):0
        sc=(n>1)?sqrt((c2-n*mc*mc)/(n-1)):0
        nl=0; for (k in l) nl++
        printf "%s: n=%d  parede=%.3f ± %.3f s  cpu=%.3f ± %.3f s  loglik distintos=%d\n", label, n, mw, sw, mc, sc, nl > "/dev/stderr"
    }')
