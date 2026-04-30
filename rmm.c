/*
============================================================================
Filename    : rmm.c
Author      : Nathan Duchosal, Nam Le
SCIPER		: , 379672
============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include "utility.h"
#include <mpi.h>

static void pack(int **matrix, int *buf, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            buf[i * cols + j] = matrix[i][j];
}

static void unpack(int *buf, int **matrix, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            matrix[i][j] = buf[i * cols + j];
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Usage: %s <M> <N> <K> <0|1>\n", argv[0]);
        return 1;
    }

    int rank, nprocs;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request reqs[3];

    /* Step 1: Read the values of M, N and K from the command line arguments. */
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int K = atoi(argv[3]);
    int *matA[M];
    int *matB[N];
    int *matC[M / 2];
    int debug = atoi(argv[4]);

    int chunk = M / nprocs;

    int *matA_contiguous = NULL;
    int *matB_contiguous = malloc(N * K * sizeof(int));
    int *matC_contiguous = NULL;
    int *local_matA = malloc(chunk * N * sizeof(int));
    int *local_matC = malloc((chunk / 2) * (K / 2) * sizeof(int));

    if (M % 2 != 0 || N % 2 != 0 || K % 2 != 0)
    {
        printf("M, N and K must be even\n");
        return 1;
    }

    if (rank == 0)
    {
        /* Step 2: Generates and initializes matrices A and B with random values. */
        init_mat(matA, M, N, 0);
        init_mat(matB, N, K, 1);
        init_mat(matC, M / 2, K / 2, -1); // -1 indicates that matrix is initialized with 0s

        matA_contiguous = malloc(M * N * sizeof(int));
        matC_contiguous = malloc((M / 2) * (K / 2) * sizeof(int));

        pack(matA, matA_contiguous, M, N);
        pack(matB, matB_contiguous, N, K);
        if (debug)
        {
            display_matrix(matA, M, N, "A");
            display_matrix(matB, N, K, "B");
        }
    }
    // only every ith row is contiguous with the way we initialised our matrix

    if (rank == 0)
    {
        printf("Starting Computation...\n");
        set_clock();
    }
    MPI_Iscatter(matA_contiguous, chunk * N, MPI_INT, local_matA, chunk * N,
                 MPI_INT, 0, MPI_COMM_WORLD, &reqs[0]);
    MPI_Ibcast(matB_contiguous, N * K, MPI_INT, 0, MPI_COMM_WORLD, &reqs[1]);
    MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);

    /* Step 3: Computes the matrix C as the RMM of matrices A and B. */
    /* Parallelize and optimize this part only! */

    for (int idx = 0; idx < (chunk / 2); idx++)
    {
        for (int jdx = 0; jdx < K / 2; jdx++)
        {
            local_matC[idx * (K / 2) + jdx] = 0;
            for (int aoff = 0; aoff < 2; aoff++)
            {
                for (int boff = 0; boff < 2; boff++)
                {
                    for (int kdx = 0; kdx < N; kdx++)
                    {
                        local_matC[idx * (K / 2) + jdx] += local_matA[(idx * 2 + aoff) * N + kdx] * matB_contiguous[kdx * K + (jdx * 2 + boff)];
                    }
                }
            }
        }
    }

    MPI_Igather(local_matC, (chunk / 2) * (K / 2), MPI_INT, matC_contiguous, (chunk / 2) * (K / 2), MPI_INT, 0, MPI_COMM_WORLD, &reqs[2]);
    MPI_Wait(&reqs[2], MPI_STATUS_IGNORE);
   
    /* Step 4: Write matrix C into a csv file matC.csv and exit. */
    if (rank == 0)
    {
        double totaltime = elapsed_time();
        unpack(matC_contiguous, matC, (M / 2), (K / 2));

        printf("Computation Done!\n");
        if (debug)
            display_matrix(matC, M / 2, K / 2, "C");
        printf("- Using %d procs: matC computed in %.4gs.\n", nprocs, totaltime);
        write_csv(matC, M / 2, K / 2, "matC.csv");

        free(matA_contiguous);
        free(matC_contiguous);
    }

    free(matB_contiguous);
    free(local_matA);
    free(local_matC);

    MPI_Finalize();
}