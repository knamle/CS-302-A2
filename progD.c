/*
============================================================================
Filename    : progD.c
Author      : Nathan Duchosal, Nam Le
SCIPER		: , 379672
============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "function.h"
#include "utility.h"
#include <mpi.h>

int main(int argc, char *argv[])
{
    int nrounds, size;

    /* Parse input arguments */
    if (argc != 3)
    {
        printf("Invalid input! Usage: ./progD <nrounds> <size>\n");
        return 1;
    }
    else
    {
        nrounds = atoi(argv[1]);
        size = atoi(argv[2]);
    }

    int rank, nprocs;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int num_workers = nprocs - 1;
    int chunk_size = size / num_workers;
    int last_chunk_size = size - chunk_size * (num_workers - 1);

    int *sendcounts = malloc(nprocs * sizeof(int));
    int *displs = malloc(nprocs * sizeof(int));

    sendcounts[0] = 0;
    displs[0] = 0;
    for (int i = 1; i < nprocs; i++)
    {
        sendcounts[i] = (i != nprocs - 1) ? chunk_size : last_chunk_size;
        displs[i] = (i - 1) * chunk_size;
    }

    int chunk = (rank == 0) ? 0 : sendcounts[rank];

    if (rank == 0)
    {
        rand_gen generator = init_rand(0);

        int *model = malloc(size * sizeof(int));
        int *nextModel = malloc(size * sizeof(int));
        int *reduceResult = malloc(size * sizeof(int));

        for (int i = 0; i < size; i++)
        {
            model[i] = next_rand(generator) * MAX_VAL;
        }

        free_rand(generator);

        set_clock();

        for (int round = 0; round < nrounds; round++)
        {
            MPI_Scatterv(model, sendcounts, displs, MPI_INT, NULL, 0, MPI_INT, 0, MPI_COMM_WORLD);

            memcpy(nextModel, model, size * sizeof(int));

            memset(reduceResult, 0, size * sizeof(int));
            MPI_Reduce(MPI_IN_PLACE, reduceResult, size, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

            for (int i = 0; i < size; i++)
            {
                nextModel[i] += reduceResult[i];
            }

            int *tmp = model;
            model = nextModel;
            nextModel = tmp;
        }

        double totaltime = elapsed_time();
        printf("- Using %d procs for %d iterations on %d size: %.3gs.\n",
               nprocs, nrounds, size, totaltime);

        write_csv(&model, 1, size, "model.csv");

        free(reduceResult);
        free(model);
        free(nextModel);
    }
    else
    {
        int *localModel = malloc(chunk * sizeof(int));
        int *localResult = malloc(size * sizeof(int));

        for (int round = 0; round < nrounds; round++)
        {
            MPI_Scatterv(NULL, NULL, NULL, MPI_INT, localModel, chunk, MPI_INT, 0, MPI_COMM_WORLD);

            memset(localResult, 0, size * sizeof(int));

            compute(localModel, localResult, chunk, size);

            MPI_Reduce(localResult, NULL, size, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        }

        free(localResult);
        free(localModel);
    }

    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}
