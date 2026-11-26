#!/bin/bash
# mede.sh EXECUTAVEL...   ex.: ./mede.sh omp-letters omp-letters-reduction ...
# Acrescenta linhas "versao,threads,rep,tempo_s" em $SAIDA (padrão resultados.csv).
# Opcional: RUN="apptainer exec hist.sif"   SAIDA=resultados-container.csv
set -u

SAIDA=${SAIDA:-resultados.csv}
RUN=${RUN:-}

[ -f "$SAIDA" ] || echo "versao,threads,rep,tempo_s" > "$SAIDA"

for exe in "$@"; do
    for p in $(printf '%s\n' 1 2 4 8 "$(nproc)" | sort -nu); do
        for r in 1 2 3 4 5; do
            t=$(OMP_NUM_THREADS=$p $RUN ./$exe < entradas/grande.txt 2>&1 >/dev/null \
                | sed -nE 's/Execution time ([0-9.]+) s.*/\1/p')
            if [ -z "$t" ]; then
                echo "FALHOU: $exe p=$p rep=$r" >&2
                continue
            fi
            echo "$exe,$p,$r,$t" >> "$SAIDA"
            printf "  %s  p=%d  rep=%d  %s s\n" "$exe" "$p" "$r" "$t"
        done
    done
done
