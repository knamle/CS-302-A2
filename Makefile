CC = mpicc
CFLAGS = -std=gnu2x -O3 -fopenmp -Wall

all: prog0 progA progB progC progD rmm rmm_openmp_mpi

prog0: prog0.c utility.h function.h
	$(CC) $(CFLAGS) $< -o $@

progA: progA.c utility.h function.h
	$(CC) $(CFLAGS) $< -o $@

progB: progB.c utility.h function.h
	$(CC) $(CFLAGS) $< -o $@

progC: progC.c utility.h function.h
	$(CC) $(CFLAGS) $< -o $@

progD: progD.c utility.h function.h
	$(CC) $(CFLAGS) $< -o $@

rmm: rmm.c utility.h
	$(CC) $(CFLAGS) $< -o $@

rmm: rmm.c utility.h
	$(CC) $(CFLAGS) $< -o $@

rmm_openmp_mpi: rmm_openmp_mpi.c utility.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f prog0 progA progB progC progD rmm rmm_openmp_mpi matC.csv model.csv
