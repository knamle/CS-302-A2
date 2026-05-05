#!/usr/bin/env bash
#SBATCH --account cs-302
#SBATCH --qos cs-302
#SBATCH --partition academic
#SBATCH --nodes 1
#SBATCH --ntasks-per-node 64
#SBATCH --cpus-per-task 1
#SBATCH --mem 32G
#SBATCH --time 01:00:00
module purge
module load gcc
module load openmpi
echo STARTING AT `date`

make partA || true

# Q1: find optimal B1 and B2 (32 procs, size=1M, 10 iterations)
echo "=== Q1: B1/B2 grid search for progB ==="
for B1 in 64 128 256 512 1024 2048 4096 8192 16384; do
    for B2 in 64 128 256 512 1024 2048 4096 8192 16384; do
        echo "B1=$B1 B2=$B2"
        srun -n 32 ./progB 10 1000000 $B1 $B2
    done
done

echo "=== Q1: B1/B2 grid search for progC ==="
for B1 in 64 128 256 512 1024 2048 4096 8192 16384; do
    for B2 in 64 128 256 512 1024 2048 4096 8192 16384; do
        echo "B1=$B1 B2=$B2"
        srun -n 32 ./progC 10 1000000 $B1 $B2
    done
done

B1=2048
B2=2048

# Q2: vary number of processes, size=1M, 10 iterations
echo "=== Q2: varying number of processes ==="
for P in 2 4 8 16 32 64; do
    echo "--- P=$P ---"
    srun -n $P ./progA 10 1000000
    srun -n $P ./progB 10 1000000 $B1 $B2
    srun -n $P ./progC 10 1000000 $B1 $B2
    srun -n $P ./progD 10 1000000
done

# Q3: vary model size, 32 processes, 10 iterations
echo "=== Q3: varying model size ==="
for S in 100 1000 10000 100000; do
    echo "--- S=$S ---"
    srun -n 32 ./progA 10 $S
    srun -n 32 ./progB 10 $S $B1 $B2
    srun -n 32 ./progC 10 $S $B1 $B2
    srun -n 32 ./progD 10 $S
done

echo FINISHED at `date`