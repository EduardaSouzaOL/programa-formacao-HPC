#!/bin/bash
# mede.sh EXECUTAVEL...   ex.: ./mede.sh letters mpi-letters-coletivas mpi-letters-p2p
# Acrescenta linhas "versao,p,rep,distribuir,contar,combinar,total" em $SAIDA
# (padrão resultados.csv), na entrada grande, 5 repetições por p.
# Na serial (letters), só a coluna total, com o tempo de make_hist().
# Na versão arquivo, a coluna distribuir guarda o tempo da fase "ler".
# p em {1, 2, 4, 8, núcleos físicos}. Acima dos núcleos físicos, usa --oversubscribe.
# Opcional: RUN="apptainer exec mpi.sif"   SAIDA=resultados-container.csv
#           PROCS="1 2 4 6"   (troca a lista de p)   MPIFLAGS="--bind-to core"
set -u

SAIDA=${SAIDA:-resultados.csv}
RUN=${RUN:-}
MPIFLAGS=${MPIFLAGS:-}
ENTRADA=${ENTRADA:-entradas/grande.txt}
FISICOS=$(lscpu -p=socket,core | grep -v '^#' | sort -u | wc -l)
PROCS=${PROCS:-$(printf '%s\n' 1 2 4 8 "$FISICOS" | sort -nu)}

[ -f "$SAIDA" ] || echo "versao,p,rep,distribuir,contar,combinar,total" > "$SAIDA"

for exe in "$@"; do
    lista=$PROCS
    [ "$exe" = letters ] && lista=1
    for p in $lista; do
        extra=""
        [ "$p" -gt "$FISICOS" ] && extra="--oversubscribe"
        for r in 1 2 3 4 5; do
            case $exe in
                letters)
                    linha=$($RUN ./letters < "$ENTRADA" 2>&1 >/dev/null \
                        | sed -nE 's/Execution time ([0-9.]+) s.*/,,,\1/p') ;;
                mpi-letters-arquivo)
                    linha=$($RUN mpirun $extra $MPIFLAGS -np "$p" ./$exe "$ENTRADA" 2>&1 >/dev/null \
                        | sed -nE 's/^ler ([0-9.]+) contar ([0-9.]+) combinar ([0-9.]+) total ([0-9.]+)/\1,\2,\3,\4/p') ;;
                *)
                    linha=$($RUN mpirun $extra $MPIFLAGS -np "$p" ./$exe < "$ENTRADA" 2>&1 >/dev/null \
                        | sed -nE 's/^distribuir ([0-9.]+) contar ([0-9.]+) combinar ([0-9.]+) total ([0-9.]+)/\1,\2,\3,\4/p') ;;
            esac
            if [ -z "$linha" ]; then
                echo "FALHOU: $exe p=$p rep=$r" >&2
                continue
            fi
            echo "$exe,$p,$r,$linha" >> "$SAIDA"
            printf "  %-22s p=%-2d rep=%d  %s\n" "$exe" "$p" "$r" "$linha"
        done
    done
done
