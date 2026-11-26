/****************************************************************************
 *
 * letters.c - Character counts
 *
 * Copyright (C) 2018--2026 Moreno Marzolla
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

/*
 * Versão serial para o desafio de MPI da Formação HPC (CESAR, 2026/2027), adaptada de
 * https://github.com/mmarzolla/parallel-etudes (omp-letters.c).
 * Modificado por Gabriel Freytag (CESAR) em 11/11/2026 e em 17/11/2026. Mudanças em
 * relação ao original:
 *   - só a versão serial (o enunciado e as soluções do original foram retirados);
 *   - lê a entrada padrão inteira, sem o limite de 5 MB;
 *   - make_hist() recebe o tamanho do texto (n), em vez de usar strlen(): um pedaço
 *     recebido por MPI_Scatter não termina em '\0';
 *   - índice do laço em size_t;
 *   - o caractere é lido como unsigned char, como pedem isalpha() e tolower();
 *   - main() confere se o total devolvido por make_hist() é a soma do histograma;
 *   - entrada vazia não gera divisão por zero em print_hist();
 *   - o tempo de make_hist() é medido com clock_gettime() (sem OpenMP) e sai na
 *     saída de erro, com 6 casas.
 * Versão MPI "arquivo" (desafio MPI, 26/11/2026, Maria Eduarda S Oliveira): muda o
 * main() e não usa read_all(). O nome do arquivo vem por argumento; cada rank abre o
 * arquivo e lê só o seu pedaço com fseeko(), sem Scatter. Nenhum processo guarda o
 * texto inteiro. A fase "ler" substitui a fase "distribuir" das outras versões.
 * O texto completo da licença está no arquivo COPYING.
 */

#define _POSIX_C_SOURCE 200809L   /* fseeko() e ftello() com -std=c99 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <mpi.h>

#define ALPHA_LEN 26

/**
 * Count occurrences of letters 'a'..'z' in the first `n` characters
 * of `text`; uppercase characters are transformed into lowercase, and
 * all other symbols are ignored. `text` does not need to be
 * zero-terminated. `hist` will be filled with the computed counts.
 * Returns the total number of letters found.
 */
int make_hist( const char *text, size_t n, int hist[ALPHA_LEN] )
{
    int nlet = 0; /* total number of alphabetic characters processed */

    /* Reset the histogram */
    for (int j=0; j<ALPHA_LEN; j++) {
        hist[j] = 0;
    }

    /* Count occurrences */
    for (size_t i=0; i<n; i++) {
        const unsigned char c = text[i];
        if (isalpha(c)) {
            const int idx = tolower(c) - 'a';
            nlet++;
            hist[idx]++;
        }
    }

    return nlet;
}

/**
 * If `freq == 100`, draw `len` caracters; otherwise, draw a fraction
 * of `len` characters proportional to `freq`.
 */
void bar( float freq, int len )
{
    for (int i=0; i<len*freq/100; i++) {
        printf("#");
    }
}

/**
 * Print frequencies
 */
void print_hist( int hist[ALPHA_LEN] )
{
    int nlet = 0;
    for (int i=0; i<ALPHA_LEN; i++) {
        nlet += hist[i];
    }
    for (int i=0; i<ALPHA_LEN; i++) {
        const float freq = (nlet > 0 ? 100.0*hist[i]/nlet : 0.0);
        printf("%c : %8d (%6.2f%%) ", 'a'+i, hist[i], freq);
        bar(freq, 65);
        printf("\n");
    }
    printf("    %8d total\n", nlet);
}

int main( int argc, char *argv[] )
{
    int rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if (argc != 2) {
        if (rank == 0) {
            fprintf(stderr, "uso: mpirun -np P %s arquivo.txt\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    long long n = 0;
    if (rank == 0) {
        FILE *f = fopen(argv[1], "rb");
        if (f == NULL || fseeko(f, 0, SEEK_END) != 0) {
            fprintf(stderr, "ERRO: não consegui abrir %s\n", argv[1]);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        n = (long long)ftello(f);
        fclose(f);
    }

    int hist[ALPHA_LEN], total[ALPHA_LEN];
    int nlet, nlet_total = 0;

    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();

    MPI_Bcast(&n, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    const long long resto = n % p;
    const size_t meu_n = (size_t)(n / p + (rank < resto));
    const long long inicio = rank * (n / p) + (rank < resto ? rank : resto);

    char *meu = (char*)malloc(meu_n > 0 ? meu_n : 1); assert(meu != NULL);
    FILE *f = fopen(argv[1], "rb");
    if (f == NULL || fseeko(f, (off_t)inicio, SEEK_SET) != 0
        || fread(meu, 1, meu_n, f) != meu_n) {
        fprintf(stderr, "ERRO: rank %d não leu %zu bytes a partir de %lld\n", rank, meu_n, inicio);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    fclose(f);
    const double t1 = MPI_Wtime();

    nlet = make_hist(meu, meu_n, hist);
    const double t2 = MPI_Wtime();

    MPI_Reduce(hist, total, ALPHA_LEN, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&nlet, &nlet_total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    const double t3 = MPI_Wtime();

    double meu_t[4] = {t1 - t0, t2 - t1, t3 - t2, t3 - t0}, pior[4];
    MPI_Reduce(meu_t, pior, 4, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    int ret = EXIT_SUCCESS;
    if (rank == 0) {
        print_hist(total);
        fprintf(stderr, "ler %.6f contar %.6f combinar %.6f total %.6f\n",
                pior[0], pior[1], pior[2], pior[3]);
        int soma = 0;
        for (int j=0; j<ALPHA_LEN; j++) {
            soma += total[j];
        }
        if (nlet_total != soma) {
            fprintf(stderr, "ERRO: make_hist() devolveu %d letras, mas o histograma soma %d\n", nlet_total, soma);
            ret = EXIT_FAILURE;
        }
    }

    free(meu);
    MPI_Finalize();
    return ret;
}
