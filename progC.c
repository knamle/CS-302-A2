/*
============================================================================
Filename    : progC.c
Author      : Nathan Duchosal, Nam Le
SCIPER		: 356203, 379672
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
        printf("Invalid input! Usage: ./progC <nrounds> <size> <B1> <B2>\n");
        return 1;
    }
    else
    {
        nrounds = atoi(argv[1]);
        size = atoi(argv[2]);
        B1 = atoi(argv[3]);
        B2 = atoi(argv[4]);
    }

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

        int *model = malloc(size * sizeof(int));
        int *nextModel = malloc(size * sizeof(int));

        for (int i = 0; i < size; i++)
        {
            model[i] = next_rand(generator) * MAX_VAL;
        }

        free_rand(generator);

        int recv_blocks_B2 = (size + B2 - 1) / B2;

        int total_isends_B1 =
            (nprocs - 2) * ((chunk_size + B1 - 1) / B1) + ((last_chunk_size + B1 - 1) / B1);

        MPI_Request *sendRequests = malloc(total_isends_B1 * sizeof(MPI_Request));
        MPI_Request *recvRequests = malloc(2 * recv_blocks_B2 * sizeof(MPI_Request));

        int *recvBufs[2];
        recvBufs[0] = malloc(size * sizeof(int));
        recvBufs[1] = malloc(size * sizeof(int));
        
        set_clock();

        for (int round = 0; round < nrounds; round++)
        {
            int req_idx_isend = 0;

            for (int i = 0; i < size; i++)
            {
                nextModel[i] = model[i];
            }

            for (int proc = 1; proc < nprocs + 1; proc++)
            {
                if (proc < nprocs)
                {
                    int slot = (proc - 1) % 2;
                    int req_idx_irecv = 0;

                    for (int i = 0; i < size; i += B2)
                    {
                        int recv_size = (i + B2 <= size) ? B2 : (size - i);

                        MPI_Irecv(
                            &recvBufs[slot][i],
                            recv_size,
                            MPI_INT,
                            proc,
                            TAG,
                            MPI_COMM_WORLD,
                            &recvRequests[slot * recv_blocks_B2 + req_idx_irecv++]);
                    }

                    int chunk = proc != (nprocs - 1) ? chunk_size : last_chunk_size;
                    int offset = (proc - 1) * chunk_size;

                    for (int i = 0; i < chunk; i += B1)
                    {
                        int send_size = (i + B1 <= chunk) ? B1 : (chunk - i);

                        MPI_Isend(
                            &model[offset + i],
                            send_size,
                            MPI_INT,
                            proc,
                            TAG,
                            MPI_COMM_WORLD,
                            &sendRequests[req_idx_isend++]);
                    }
                }

                if (proc > 1)
                {
                    int prev_slot = (proc - 2) % 2;

                    MPI_Waitall(
                        recv_blocks_B2,
                        &recvRequests[prev_slot * recv_blocks_B2],
                        MPI_STATUSES_IGNORE);

                    for (int s = 0; s < size; s++)
                    {
                        nextModel[s] += recvBufs[prev_slot][s];
                    }
                }
            }

            MPI_Waitall(req_idx_isend, sendRequests, MPI_STATUSES_IGNORE);

            int *tmp = model;
            model = nextModel;
            nextModel = tmp;
        }
        double totaltime = elapsed_time();
        printf("- Using %d procs for %d iterations on %d size: %.3gs.\n",
               nprocs, nrounds, size, totaltime);

        write_csv(&model, 1, size, "model.csv");

        free(recvBufs[0]);
        free(recvBufs[1]);
        free(recvRequests);
        free(sendRequests);

        free(model);
        free(nextModel);
    }
    else
    {
        int chunk = rank != (nprocs - 1) ? chunk_size : last_chunk_size;

        int *localModel = malloc(chunk * sizeof(int));
        int *localResult = malloc(size * sizeof(int));

        int total_irecv_B1 = (chunk + B1 - 1) / B1;
        int total_isend_B2 = (size + B2 - 1) / B2;

        MPI_Request *recvRequests = malloc(total_irecv_B1 * sizeof(MPI_Request));
        MPI_Request *sendRequests = malloc(total_isend_B2 * sizeof(MPI_Request));

        for (int round = 0; round < nrounds; round++)
        {
            int req_idx = 0;

            for (int i = 0; i < chunk; i += B1)
            {
                int recv_size = (i + B1 <= chunk) ? B1 : (chunk - i);

                MPI_Irecv(
                    &localModel[i],
                    recv_size,
                    MPI_INT,
                    0,
                    TAG,
                    MPI_COMM_WORLD,
                    &recvRequests[req_idx++]);
            }

            for (int i = 0; i < size; i++)
            {
                localResult[i] = 0;
            }

            int completed = 0;

            while (completed < total_irecv_B1)
            {
                int index;

                MPI_Waitany(
                    total_irecv_B1,
                    recvRequests,
                    &index,
                    MPI_STATUS_IGNORE);

                if (index == MPI_UNDEFINED)
                {
                    break;
                }

                int offset = index * B1;
                int block_size = (offset + B1 <= chunk) ? B1 : (chunk - offset);

                compute(&localModel[offset], localResult, block_size, size);

                completed++;
            }

            req_idx = 0;

            for (int i = 0; i < size; i += B2)
            {
                int send_size = (i + B2 <= size) ? B2 : (size - i);

                MPI_Isend(
                    &localResult[i],
                    send_size,
                    MPI_INT,
                    0,
                    TAG,
                    MPI_COMM_WORLD,
                    &sendRequests[req_idx++]);
            }

            MPI_Waitall(total_isend_B2, sendRequests, MPI_STATUSES_IGNORE);
        }

        free(sendRequests);
        free(recvRequests);
        free(localResult);
        free(localModel);
    }
    MPI_Finalize();
    return 0;
}