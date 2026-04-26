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

    /* Step 1: Read the values of M, N and K from the command line arguments. */
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int K = atoi(argv[3]);
    int debug = atoi(argv[4]);

    int chunk = M / nprocs;

    if (M % 2 != 0 || N % 2 != 0 || K % 2 != 0)
    {
        printf("M, N and K must be even\n");
        return 1;
    }

    if (rank == 0)
    {
        /* Step 2: Generates and initializes matrices A and B with random values. */
        int *matA[M];
        int *matB[N];
        int *matC[M / 2];

        init_mat(matA, M, N, 0);
        init_mat(matB, N, K, 1);
        init_mat(matC, M / 2, K / 2, -1); // -1 indicates that matrix is initialized with 0s

        int **result = malloc((M / 2) * sizeof(int *));
        for (int i = 0; i < M / 2; i++)
        {
            result[i] = malloc((K / 2) * sizeof(int))
        }

        if (debug)
        {
            display_matrix(matA, M, N, "A");
            display_matrix(matB, N, K, "B");
        }
    }

    int **local_matA = malloc(chunk * sizeof(int *));
    for (int i = 0; i < chunk; i++)
    {
        local_matA[i] = malloc(N * sizeof(int));
    }
    MPI_Scatter(matA, chunk, MPI_INT, local_matA, chunk, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(matB, N * K, MPI_INT, 0, MPI_COMM_WORLD);

    printf("Starting Computation...\n");
    set_clock();

    /* Step 3: Computes the matrix C as the RMM of matrices A and B. */
    /* Parallelize and optimize this part only! */
    int **local_matC = malloc((chunk / 2) * sizeof(int *));
    for (int i = 0; i < (chunk / 2); i++)
    {
        local_matC[i] = malloc((K / 2) * sizeof(int))
    }

    for (int idx = 0; idx < (chunk / 2); idx++)
    {
        for (int jdx = 0; jdx < K / 2; jdx++)
        {
            local_matC[idx][jdx] = 0;
            for (int aoff = 0; aoff < 2; aoff++)
            {
                for (int boff = 0; boff < 2; boff++)
                {
                    for (int kdx = 0; kdx < N; kdx++)
                    {
                        local_matC[idx][jdx] += local_matA[idx * 2 + aoff][kdx] * matB[kdx][jdx * 2 + boff];
                    }
                }
            }
        }
    }

    MPI_Gather(local_matC, chunk, MPI_INT, matC, chunk, MPI_INT, 0, MPI_COMM_WORLD);
    double totaltime = elapsed_time();
    
    /* Step 4: Write matrix C into a csv file matC.csv and exit. */
    if (rank == 0)
    {
        printf("Computation Done!\n");
        if (debug)
            display_matrix(matC, M / 2, K / 2, "C");
        printf("- Using %d procs: matC computed in %.4gs.\n", nprocs, totaltime);
        write_csv(matC, M / 2, K / 2, "matC.csv");

        /*free(matA);
        free(matB);
        free(matC);*/
        
        for (int i = 0; i < M/2 ; i++) {
            free(result[i]);
        }
        free(result);
    }

    for (int i = 0; i < chunk ; i++) {
            free(local_matA[i]);
    }
    for (int i = 0; i < chunk/2 ; i++) {
            free(local_matC[i]);
    }
    free(local_matA);
    free(local_matC);

    MPI_Finalize();
}