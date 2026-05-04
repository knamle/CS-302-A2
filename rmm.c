/*
============================================================================
Filename    : rmm.c
Author      : Nathan Duchosal, Nam Le
SCIPER      : 356203, 379672
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

static void pack_B_tile(const int *full_B, int *tile, int N, int K,
                        int col_start, int tile_w)
{
    for (int r = 0; r < N; r++)
        for (int c = 0; c < tile_w; c++)
            tile[r * tile_w + c] = full_B[r * K + col_start + c];
}

static void compute_tile(const int *local_matA, const int *B_tile,
                         int *local_matC,
                         int chunk, int N, int K,
                         int col_start, int tile_w)
{
    int half_tile = tile_w / 2;
    int half_K = K / 2;

    for (int idx = 0; idx < chunk / 2; idx++)
    {
        for (int jdx = 0; jdx < half_tile; jdx++)
        {
            int c_col = col_start / 2 + jdx;
            for (int aoff = 0; aoff < 2; aoff++)
                for (int boff = 0; boff < 2; boff++)
                    for (int kdx = 0; kdx < N; kdx++)
                        local_matC[idx * half_K + c_col] +=
                            local_matA[(idx * 2 + aoff) * N + kdx] *
                            B_tile[kdx * tile_w + (jdx * 2 + boff)];
        }
    }
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

    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int K = atoi(argv[3]);
    int debug = atoi(argv[4]);
    int T = 64; // number of columns that matB will have workers work on

    int *matA[M];
    int *matB[N];
    int *matC[M / 2];

    if (M % 2 != 0 || N % 2 != 0 || K % 2 != 0)
    {
        printf("M, N and K must be even\n");
        MPI_Finalize();
        return 1;
    }

    int chunk = M / nprocs;
    int chunk_c_size = (chunk / 2) * (K / 2);
    int tile_w = K / T; // tile width
    int tile_size = N * tile_w;

    int *local_matA = malloc(chunk * N * sizeof(int));
    int *local_matC = calloc(chunk_c_size, sizeof(int));
    int *B_buf[2];
    B_buf[0] = malloc(tile_size * sizeof(int));
    B_buf[1] = malloc(tile_size * sizeof(int));

    /* ------------------------------------------------------------------ */
    /* RANK 0 setup                                                        */
    /* ------------------------------------------------------------------ */
    int *matA_flat = NULL;
    int *matB_flat = NULL;
    int *matC_flat = NULL;

    if (rank == 0)
    {
        init_mat(matA, M, N, 0);
        init_mat(matB, N, K, 1);
        init_mat(matC, M / 2, K / 2, -1);

        matA_flat = malloc(M * N * sizeof(int));
        matB_flat = malloc(N * K * sizeof(int));
        matC_flat = malloc((M / 2) * (K / 2) * sizeof(int));

        pack(matA, matA_flat, M, N);
        pack(matB, matB_flat, N, K);

        if (debug)
        {
            display_matrix(matA, M, N, "A");
            display_matrix(matB, N, K, "B");
        }

        printf("Starting Computation...\n");
        set_clock();

        // recv back from all workers
        MPI_Request *c_recv_reqs = malloc((nprocs - 1) * sizeof(MPI_Request));
        for (int r = 1; r < nprocs; r++)
        {
            int offset = (r * chunk / 2) * (K / 2);
            MPI_Irecv(matC_flat + offset, chunk_c_size, MPI_INT,
                      r, 2, MPI_COMM_WORLD, &c_recv_reqs[r - 1]);
        }

        // send chunks of A to workers
        MPI_Request *a_send_reqs = malloc((nprocs - 1) * sizeof(MPI_Request));
        for (int r = 1; r < nprocs; r++)
        {
            MPI_Isend(matA_flat + r * chunk * N, chunk * N, MPI_INT,
                      r, 0, MPI_COMM_WORLD, &a_send_reqs[r - 1]);
        }

        // sends chunks of B to workers
        int *tile_bufs[T];
        MPI_Request *b_send_reqs = malloc((nprocs - 1) * T * sizeof(MPI_Request));

        for (int t = 0; t < T; t++)
        {
            tile_bufs[t] = malloc(tile_size * sizeof(int));
            pack_B_tile(matB_flat, tile_bufs[t], N, K, t * tile_w, tile_w);
            for (int r = 1; r < nprocs; r++)
            {
                MPI_Isend(tile_bufs[t], tile_size, MPI_INT,
                          r, 10 + t, MPI_COMM_WORLD,
                          &b_send_reqs[t * (nprocs - 1) + (r - 1)]);
            }
        }

        // rank0 computes its own chunk
        for (int t = 0; t < T; t++)
        {
            pack_B_tile(matB_flat, B_buf[0], N, K, t * tile_w, tile_w);
            compute_tile(matA_flat,
                         B_buf[0],
                         matC_flat,
                         chunk, N, K,
                         t * tile_w, tile_w);
        }

        MPI_Waitall((nprocs - 1) * T, b_send_reqs, MPI_STATUSES_IGNORE);
        MPI_Waitall(nprocs - 1, a_send_reqs, MPI_STATUSES_IGNORE);
        MPI_Waitall(nprocs - 1, c_recv_reqs, MPI_STATUSES_IGNORE);

        double totaltime = elapsed_time();

        unpack(matC_flat, matC, M / 2, K / 2);

        printf("Computation Done!\n");
        if (debug)
            display_matrix(matC, M / 2, K / 2, "C");
        printf("- Using %d procs: matC computed in %.4gs.\n",
               nprocs, totaltime);
        write_csv(matC, M / 2, K / 2, "matC.csv");

        free(matA_flat);
        free(matB_flat);
        free(matC_flat);
        free(a_send_reqs);
        free(b_send_reqs);
        free(c_recv_reqs);
        for (int t = 0; t < T; t++)
        {
            free(tile_bufs[t]);
        }
    }
    /* ------------------------------------------------------------------ */
    /* WORKERS: receive A once, then pipeline B tiles                      */
    /* ------------------------------------------------------------------ */
    else
    {
        // Receive A's chunk
        MPI_Recv(local_matA, chunk * N, MPI_INT, 0, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // t0 = recv B0
        // t1 = recv B1 and compute B0 etc..
        int cur = 0;
        MPI_Request b_req = MPI_REQUEST_NULL;

        MPI_Irecv(B_buf[cur], tile_size, MPI_INT, 0, 10,
                  MPI_COMM_WORLD, &b_req);

        for (int t = 0; t < T; t++)
        {
            int next = cur ^ 1;

            // post next recv
            MPI_Request next_req = MPI_REQUEST_NULL;
            if (t + 1 < T)
                MPI_Irecv(B_buf[next], tile_size, MPI_INT, 0, 10 + t + 1,
                          MPI_COMM_WORLD, &next_req);

            // wait for last receive
            MPI_Wait(&b_req, MPI_STATUS_IGNORE);

            // compute last received data
            compute_tile(local_matA, B_buf[cur], local_matC,
                         chunk, N, K, t * tile_w, tile_w);

            b_req = next_req;
            cur = next;
        }
        MPI_Send(local_matC, chunk_c_size, MPI_INT, 0, 2, MPI_COMM_WORLD);
    }

    free(local_matA);
    free(local_matC);
    free(B_buf[0]);
    free(B_buf[1]);

    MPI_Finalize();
    return 0;
}