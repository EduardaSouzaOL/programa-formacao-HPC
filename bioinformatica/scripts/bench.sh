#!/usr/bin/env bash
# Repete uma execução do IQ-TREE N vezes e resume log-likelihood e tempos.
#
# Uso:    scripts/bench.sh <rotulo> <N> <comando...>
# Saída:  linhas CSV (rotulo,rodada,loglik,parede_s,cpu_s,externo_s) em stdout
#         e média/desvio-padrão em stderr, ao final.
#
#   parede_s, cpu_s: tempos reportados pelo IQ-TREE (não incluem a partida do contêiner)
#   externo_s:       tempo medido por fora, do início ao fim do comando
#                    (inclui a partida do Docker/Apptainer)
#
# O comando deve incluir -redo, para o IQ-TREE não reaproveitar o checkpoint.
# Exemplos:
#   scripts/bench.sh nativo 10 third_party/iqtree2/build/iqtree2 \
#       -s data/example.phy -m GTR+G -T 2 --seed 1 --prefix data/nativo -redo
#   scripts/bench.sh docker 10 docker run --rm --user "$(id -u):$(id -g)" \
#       -v "$PWD/data":/data iqtree2:2.4.0 \
#       -s example.phy -m GTR+G -T 2 --seed 1 --prefix docker -redo
#   scripts/bench.sh apptainer 10 apptainer run --pwd /data -B "$PWD/data":/data iqtree2.sif \
#       -s example.phy -m GTR+G -T 2 --seed 1 --prefix apptainer -redo
set -euo pipefail

# Em locale com vírgula decimal (pt_BR etc.), o awk imprime tempos com vírgula,
# que viram separador no CSV e bagunçam o resumo. Força ponto decimal no awk e no date.
export LC_NUMERIC=C

if [ "$#" -lt 3 ]; then
    sed -n '/^#!/d; /^#/!q; s/^# \{0,1\}//; p' "$0" >&2
    exit 1
fi

label=$1; n=$2; shift 2
out=$(mktemp)   # saída de cada rodada
csv=$(mktemp)   # linhas CSV acumuladas
trap 'rm -f "$out" "$csv"' EXIT

# Extrai o 5º campo da primeira linha que casa com o padrão (vazio se não houver).
campo() { grep -m1 "$1" "$out" | awk '{print $5}' || true; }

for i in $(seq 1 "$n"); do
    t0=$(date +%s.%N)
    "$@" > "$out" 2>&1 || { echo "Falha na rodada $i:" >&2; tail -n 5 "$out" >&2; exit 1; }
    t1=$(date +%s.%N)

    lnl=$(campo "BEST SCORE FOUND")
    wall=$(campo "Total wall-clock time used")
    cpu=$(campo "Total CPU time used")
    if [ -z "$lnl" ] || [ -z "$wall" ] || [ -z "$cpu" ]; then
        echo "Rodada $i: não encontrei log-likelihood ou tempos na saída:" >&2
        tail -n 5 "$out" >&2
        exit 1
    fi
    ext=$(awk -v a="$t0" -v b="$t1" 'BEGIN { printf "%.3f", b - a }')

    echo "${label},${i},${lnl},${wall},${cpu},${ext}" | tee -a "$csv"
done

# Resumo (média e desvio-padrão amostral, em duas passadas para evitar erro numérico)
awk -F, -v label="$label" '
    { n++; l[$3]++; w[n]=$4; c[n]=$5; e[n]=$6; sw+=$4; sc+=$5; se+=$6 }
    END {
        mw=sw/n; mc=sc/n; me=se/n
        for (i = 1; i <= n; i++) {
            dw += (w[i]-mw)^2; dc += (c[i]-mc)^2; de += (e[i]-me)^2
        }
        if (n > 1) { dw = sqrt(dw/(n-1)); dc = sqrt(dc/(n-1)); de = sqrt(de/(n-1)) }
        else       { dw = 0; dc = 0; de = 0 }
        nl = 0; for (k in l) nl++
        printf "%s: n=%d  parede=%.3f ± %.3f s  cpu=%.3f ± %.3f s  externo=%.3f ± %.3f s  loglik distintos=%d\n", \
               label, n, mw, dw, mc, dc, me, de, nl > "/dev/stderr"
    }' "$csv"
