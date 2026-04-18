#!/usr/bin/env bash
#SBATCH --account cs-302
#SBATCH --qos cs-302
#SBATCH --partition academic
#SBATCH --nodes 1
#SBATCH --ntasks-per-node 32
#SBATCH --cpus-per-task 1
#SBATCH --mem 32G
#SBATCH --time 00:30:00
module purge
module load gcc
module load openmpi
echo STARTING AT `date`
# compile
make || true
# execute
#srun ./progA 10 1000000
srun ./progB 10 1000000 10 100

echo FINISHED at `date`