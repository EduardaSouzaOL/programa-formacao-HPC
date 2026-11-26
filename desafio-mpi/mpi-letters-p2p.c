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
 * Versão MPI "p2p" (desafio MPI, 26/11/2026, Maria Eduarda S Oliveira): muda só o
 * main(). Mesma divisão da versão "coletivas", mas o rank 0 distribui os pedaços e
 * recolhe os histogramas só com MPI_Send e MPI_Recv. A medição usa MPI_Barrier e
 * MPI_Reduce, como na outra versão.
 * O texto completo da licença está no arquivo COPYING.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <mpi.h>

#define TAG_TAMANHO 1
#define TAG_PEDACO  2
#define TAG_HIST    3

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

/**
 * Read all of standard input into a zero-terminated buffer.
 */
char *read_all( size_t *len )
{
    size_t cap = 1024*1024, n = 0, r;
    char *buf = (char*)malloc(cap); assert(buf != NULL);
    while ((r = fread(buf + n, 1, cap - n - 1, stdin)) > 0) {
        n += r;
        if (cap - n - 1 == 0) {
            cap *= 2;
            buf = (char*)realloc(buf, cap); assert(buf != NULL);
        }
    }
    buf[n] = '\0'; /* put a termination mark at the end of the text */
    *len = n;
    return buf;
}

int main( int argc, char *argv[] )
{
    int rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    char *text = NULL;
    size_t len = 0;
    if (rank == 0) {
        text = read_all(&len);
        if (len > INT_MAX) {
            fprintf(stderr, "ERRO: entrada com %zu bytes, o limite é %d\n", len, INT_MAX);
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    /* hist[ALPHA_LEN] guarda o total de letras, para viajar na mesma mensagem */
    int hist[ALPHA_LEN + 1], total[ALPHA_LEN + 1];
    char *meu = NULL;

    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();

    int n = (int)len;
    if (rank == 0) {
        for (int r=1; r<p; r++) {
            MPI_Send(&n, 1, MPI_INT, r, TAG_TAMANHO, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(&n, 1, MPI_INT, 0, TAG_TAMANHO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    const int meu_n = n / p + (rank < n % p);
    if (rank == 0) {
        /* o pedaço do rank 0 é o começo do texto: não precisa de cópia */
        int desl = meu_n;
        for (int r=1; r<p; r++) {
            const int cont = n / p + (r < n % p);
            MPI_Send(text + desl, cont, MPI_CHAR, r, TAG_PEDACO, MPI_COMM_WORLD);
            desl += cont;
        }
    } else {
        meu = (char*)malloc(meu_n > 0 ? meu_n : 1); assert(meu != NULL);
        MPI_Recv(meu, meu_n, MPI_CHAR, 0, TAG_PEDACO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    const double t1 = MPI_Wtime();

    hist[ALPHA_LEN] = make_hist(rank == 0 ? text : meu, meu_n, hist);
    const double t2 = MPI_Wtime();

    if (rank == 0) {
        memcpy(total, hist, sizeof(total));
        for (int r=1; r<p; r++) {
            int outro[ALPHA_LEN + 1];
            MPI_Recv(outro, ALPHA_LEN + 1, MPI_INT, MPI_ANY_SOURCE, TAG_HIST,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j=0; j<=ALPHA_LEN; j++) {
                total[j] += outro[j];
            }
        }
    } else {
        MPI_Send(hist, ALPHA_LEN + 1, MPI_INT, 0, TAG_HIST, MPI_COMM_WORLD);
    }
    const double t3 = MPI_Wtime();

    double meu_t[4] = {t1 - t0, t2 - t1, t3 - t2, t3 - t0}, pior[4];
    MPI_Reduce(meu_t, pior, 4, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    int ret = EXIT_SUCCESS;
    if (rank == 0) {
        print_hist(total);
        fprintf(stderr, "distribuir %.6f contar %.6f combinar %.6f total %.6f\n",
                pior[0], pior[1], pior[2], pior[3]);
        int soma = 0;
        for (int j=0; j<ALPHA_LEN; j++) {
            soma += total[j];
        }
        if (total[ALPHA_LEN] != soma) {
            fprintf(stderr, "ERRO: make_hist() devolveu %d letras, mas o histograma soma %d\n", total[ALPHA_LEN], soma);
            ret = EXIT_FAILURE;
        }
    }

    free(meu);
    free(text);
    MPI_Finalize();
    return ret;
}
