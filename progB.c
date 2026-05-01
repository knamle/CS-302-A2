/*
============================================================================
Filename    : progB.c
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

int main(int argc, char *argv[]) {
    int nrounds, size, B1, B2;

    /* Parse input arguments */
    if(argc != 5) {
        printf("Invalid input! Usage: ./progB <nrounds> <size> <B1> <B2>\n");
        return 1;
    } else {
        nrounds = atoi(argv[1]);
        size = atoi(argv[2]);
        B1 = atoi(argv[3]);
        B2 = atoi(argv[4]);
    }

    /* Get the number of processes */
    int rank, nprocs;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int chunk_size = size / (nprocs - 1);
    int last_chunk_size = size - chunk_size * (nprocs-2);

    if(rank == 0){
        /* Initialise model */
        rand_gen generator = init_rand(0);
        int *model = (int*) malloc(size * sizeof(int));
        for(int i = 0; i < size; i++) {
            model[i] = next_rand(generator) * MAX_VAL;
        }
        free_rand(generator);

        set_clock();
        for(int round = 0; round < nrounds; round++) {
            for (int proc = 1; proc < nprocs; proc++) {
                int chunk = proc != (nprocs-1) ? chunk_size: last_chunk_size;
                int offset = (proc-1)*chunk_size;
                
                for (int i = 0; i < chunk; i += B1) {
                    int send_size = (i + B1 <= chunk) ? B1 : (chunk - i);
                    MPI_Send(&model[offset + i], send_size, MPI_INT, proc, TAG, MPI_COMM_WORLD);
                }
            }

            for (int proc = 1; proc < nprocs; proc++) {
                int *recvBuf = malloc(size * sizeof(int));
                for (int i = 0; i < size; i += B2) {
                    int recv_size = (i + B2 <= size) ? B2 : (size - i);
                    MPI_Recv(&recvBuf[i], recv_size, MPI_INT, proc, TAG, MPI_COMM_WORLD, &status);
                }
                for (int s = 0; s < size; s++) {
                    model[s] += recvBuf[s];
                }
                free(recvBuf);
            }
        }
        /* Output stats */
        double totaltime = elapsed_time();
        printf("- Using %d procs for %d iterations on %d size: %.3gs.\n", nprocs, nrounds, size, totaltime);
        write_csv(&model, 1, size, "model.csv");
        
    } else {
        int chunk = rank != (nprocs-1) ? chunk_size: last_chunk_size;

        int *localModel = malloc(chunk * sizeof(int));
        int *localResult = NULL; 
        for(int round = 0; round < nrounds; round++) {
            for (int i = 0; i < chunk; i += B1) {
                    int rec_size = (i + B1 <= chunk) ? B1 : (chunk - i);
                    MPI_Recv(&localModel[i], rec_size, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
            }

            localResult = calloc(size, sizeof(int));
            compute(localModel, localResult, chunk, size);
            
            for (int i = 0; i < size; i += B2) {
                int send_size = (i + B2 <= size) ? B2 : (size - i);
                MPI_Send(&localResult[i], send_size, MPI_INT, 0, TAG, MPI_COMM_WORLD);
            }

            free(localResult);
        }
        free(localModel);
    }
    MPI_Finalize();
    return 0;
}