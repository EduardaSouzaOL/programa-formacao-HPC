#!/bin/bash
# confere.sh [VERSAO...]  -  valida cada versão contra a serial na entrada média.
# Sem argumentos, confere todas as versões padrão. Para cada versão e cada p em
# {1, 2, 4, 8, nproc}, roda 5 vezes e espera 5/5 saídas iguais à serial. A ingênua
# costuma acertar com p=1 (não há corrida) e falhar com p>=2.
set -u

VERSOES=${@:-"ingenua critical atomic local reduction matriz matriz-pad"}

./omp-letters < entradas/media.txt > serial.out 2>/dev/null
if [ ! -s serial.out ]; then
    echo "serial.out vazio: tente 'make omp-letters' e rode de novo" >&2
    exit 1
fi

for v in $VERSOES; do
    bin="./omp-letters-$v"
    if [ ! -x "$bin" ]; then
        echo "$v: $bin não existe. Rode 'make omp-letters-$v' antes." >&2
        continue
    fi
    for p in $(printf '%s\n' 1 2 4 8 "$(nproc)" | sort -nu); do
        ok=0
        for r in 1 2 3 4 5; do
            OMP_NUM_THREADS=$p $bin < entradas/media.txt > par.out 2>/dev/null \
                && diff -q serial.out par.out >/dev/null && ok=$((ok+1))
        done
        printf "%-12s p=%-2d  %d/5\n" "$v" "$p" "$ok"
    done
done
rm -f par.out
