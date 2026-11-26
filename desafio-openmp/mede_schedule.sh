#!/bin/bash
# mede_schedule.sh  -  laboratório para a pergunta 3.
# Roda omp-letters-sched com OMP_SCHEDULE variando, na entrada média, 3 vezes.
# Precisa de omp-letters-sched compilado (make omp-letters-sched).
set -u

exe=./omp-letters-sched
entrada=entradas/media.txt
p=${OMP_NUM_THREADS:-$(nproc)}

echo "versao=sched  threads=$p  entrada=$entrada  (mediana de 3 execuções)"
for s in "static" "static,1" "static,4096" "dynamic,1" "dynamic,64" "dynamic,4096" "dynamic,65536" "guided" "guided,64"; do
    tempos=()
    for r in 1 2 3; do
        t=$(OMP_SCHEDULE=$s OMP_NUM_THREADS=$p $exe < $entrada 2>&1 >/dev/null \
            | sed -nE 's/Execution time ([0-9.]+) s.*/\1/p')
        tempos+=($t)
    done
    med=$(printf '%s\n' "${tempos[@]}" | sort -n | awk 'NR==2')
    printf "  OMP_SCHEDULE=%-15s  %s s  (de: %s)\n" "$s" "$med" "${tempos[*]}"
done
