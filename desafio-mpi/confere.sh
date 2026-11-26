#!/bin/bash
# confere.sh [VERSAO...]  -  valida cada versão MPI contra a serial.
# Sem argumentos, confere coletivas, p2p e arquivo. Na entrada média, roda com
# p em {1, 2, 3, 4, 8, nproc} (o 3 não divide o texto) e compara com diff.
# Depois, os casos extremos: arquivo vazio e "abc" com 4 e 8 processos, e a
# entrada pequena com 1 processo.
# Opcional: RUN="apptainer exec mpi.sif"
set -u

VERSOES=${@:-"coletivas p2p arquivo"}
RUN=${RUN:-}
# --oversubscribe: deixa passar de um processo por núcleo físico
MPIRUN="$RUN mpirun --oversubscribe"

: > vazio.txt
printf "abc" > tres.txt

# roda VERSAO P ENTRADA, com a saída em par.out
roda() {
    if [ "$1" = arquivo ]; then
        $MPIRUN -np "$2" ./mpi-letters-arquivo "$3" > par.out 2>/dev/null
    else
        $MPIRUN -np "$2" ./mpi-letters-"$1" < "$3" > par.out 2>/dev/null
    fi
}

# confere VERSAO P ENTRADA: imprime ok ou FALHOU
confere() {
    $RUN ./letters < "$3" > serial.out 2>/dev/null
    if roda "$1" "$2" "$3" && diff -q serial.out par.out > /dev/null; then
        printf "%-10s p=%-2d %-22s ok\n" "$1" "$2" "$3"
    else
        printf "%-10s p=%-2d %-22s FALHOU\n" "$1" "$2" "$3"
        falhas=$((falhas+1))
    fi
}

falhas=0
for v in $VERSOES; do
    if [ ! -x "./mpi-letters-$v" ]; then
        echo "$v: ./mpi-letters-$v não existe. Rode 'make mpi-letters-$v' antes." >&2
        falhas=$((falhas+1))
        continue
    fi
    for p in $(printf '%s\n' 1 2 3 4 8 "$(nproc)" | sort -nu); do
        confere "$v" "$p" entradas/media.txt
    done
    confere "$v" 4 vazio.txt
    confere "$v" 4 tres.txt
    confere "$v" 8 tres.txt
    confere "$v" 1 entradas/pequena.txt
done
rm -f par.out
echo "falhas: $falhas"
[ "$falhas" -eq 0 ]
