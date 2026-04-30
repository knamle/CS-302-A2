/*
============================================================================
Filename    : progC.c
Author      : Nathan Duchosal, Nam Le
SCIPER		: , 379672
============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include "function.h"
#include "utility.h"
#include <mpi.h>

#define TAG 0

int main(int argc, char *argv[])
{
    int nrounds, size, B1, B2;

    /* Parse input arguments */
    if (argc != 5)
    {
        printf("Invalid input! Usage: ./progB <nrounds> <size> <B1> <B2>\n");
        return 1;
    }
    else
    {
        nrounds = atoi(argv[1]);
        size = atoi(argv[2]);
        B1 = atoi(argv[3]);
        B2 = atoi(argv[4]);
    }

    /* Get the number of processes */
    int rank, nprocs;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int chunk_size = size / (nprocs - 1);
    int last_chunk_size = size - chunk_size * (nprocs - 2);

    if (rank == 0)
    {
        /* Initialise model */
        rand_gen generator = init_rand(0);
        int *model = (int *)malloc(size * sizeof(int));
        for (int i = 0; i < size; i++)
        {
            model[i] = next_rand(generator) * MAX_VAL;
        }
        free_rand(generator);

        // isend
        int total_isends_B1 = (nprocs - 2) * ((chunk_size + B1 - 1) / B1) // all procs except last
                              + (last_chunk_size + B1 - 1) / B1;          // + last proc
        MPI_Request *requests = malloc(total_isends_B1 * sizeof(MPI_Request));
       
        // irecv
        int total_irecv_B2 = (nprocs - 1) * ((size + B2 - 1) / B2);
        MPI_Request *requests2 = malloc(total_irecv_B2 * sizeof(MPI_Request));
        int **recvBufs = malloc((nprocs - 1) * sizeof(int *));

        set_clock();

        for (int round = 0; round < nrounds; round++)
        {
             int req_idx = 0;

            for (int proc = 1; proc < nprocs; proc++)
            {
                int chunk = proc != (nprocs - 1) ? chunk_size : last_chunk_size;
                int offset = (proc - 1) * chunk_size;

                for (int i = 0; i < chunk; i += B1)
                {
                    int send_size = (i + B1 <= chunk) ? B1 : (chunk - i);
                    MPI_Isend(&model[offset + i], send_size, MPI_INT, proc, TAG, MPI_COMM_WORLD, &requests[req_idx++]);
                }
            }

            req_idx = 0;

            for (int proc = 1; proc < nprocs; proc++)
            {
                recvBufs[proc - 1] = malloc(size * sizeof(int));
                for (int i = 0; i < size; i += B2)
                {
                    int recv_size = (i + B2 <= size) ? B2 : (size - i);
                    MPI_Irecv(&recvBufs[proc - 1][i], recv_size, MPI_INT, proc, TAG, MPI_COMM_WORLD, &requests2[req_idx++]);
                }
            }

            MPI_Waitall(total_isends_B1, requests, MPI_STATUSES_IGNORE);
            MPI_Waitall(total_irecv_B2, requests2, MPI_STATUSES_IGNORE);

            for (int proc = 1; proc < nprocs; proc++)
            {
                for (int s = 0; s < size; s++)
                    model[s] += recvBufs[proc - 1][s];
                free(recvBufs[proc - 1]);
            }
        }

        free(recvBufs);
        free(requests);
        free(requests2);

        /* Output stats */
        double totaltime = elapsed_time();
        printf("- Using %d procs for %d iterations on %d size: %.3gs.\n", nprocs, nrounds, size, totaltime);
        write_csv(&model, 1, size, "model.csv");
    }
    else
    {
        int chunk = rank != (nprocs - 1) ? chunk_size : last_chunk_size;

        int *localModel = malloc(chunk * sizeof(int));
        int *localResult = NULL;
        for (int round = 0; round < nrounds; round++)
        { // irecv
            int total_irecv_B1 = (chunk + B1 - 1) / B1;
            MPI_Request *requests3 = malloc(total_irecv_B1 * sizeof(MPI_Request));
            int req_inx = 0;

            for (int i = 0; i < chunk; i += B1)
            {
                int rec_size = (i + B1 <= chunk) ? B1 : (chunk - i);
                MPI_Irecv(&localModel[i], rec_size, MPI_INT, 0, TAG, MPI_COMM_WORLD, &requests3[req_inx++]);
            }

            localResult = calloc(size, sizeof(int));

            MPI_Waitall(total_irecv_B1, requests3, MPI_STATUSES_IGNORE);
            free(requests3);

            compute(localModel, localResult, chunk, size);

            // isend
            int total_isend_B2 = ((size + B2 - 1) / B2);
            MPI_Request *requests4 = malloc(total_isend_B2 * sizeof(MPI_Request));
            req_inx = 0;

            for (int i = 0; i < size; i += B2)
            {
                int send_size = (i + B2 <= size) ? B2 : (size - i);
                MPI_Isend(&localResult[i], send_size, MPI_INT, 0, TAG, MPI_COMM_WORLD, &requests4[req_inx++]);
                localResult[i] = 0;
            }

            MPI_Waitall(total_isend_B2, requests4, MPI_STATUSES_IGNORE);
            free(requests4);

            free(localResult);
        }
        free(localModel);
    }
    MPI_Finalize();
    return 0;
}