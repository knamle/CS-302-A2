#!/usr/bin/env bash
#SBATCH --account cs-302
#SBATCH --qos cs-302
#SBATCH --partition academic
#SBATCH --nodes 1
#SBATCH --ntasks 16
#SBATCH --cpus-per-task 4
#SBATCH --mem-per-cpu 6000
#SBATCH --time 00:30:00
module purge
module load gcc
module load openmpi
echo STARTING AT `date`

make rmm rmm_openmp_mpi || true

echo "=== rmm (MPI only, point-to-point), M=N=K=4096 ==="
for P in 2 4 8 16; do
    echo "--- P=$P ---"
    srun -n $P ./rmm 4096 4096 4096 0
done

echo "=== rmm_openmp_mpi (collectives + OpenMP), M=N=K=4096 ==="
for P in 2 4 8 16; do
    for THR in 1 2 4; do
        TOTAL=$((P * THR))
        if [ $TOTAL -le 64 ]; then
            echo "--- P=$P, threads=$THR ---"
            srun -n $P --cpus-per-task $THR ./rmm_openmp_mpi 4096 4096 4096 0 $THR
        fi
    done
done

echo FINISHED at `date`
