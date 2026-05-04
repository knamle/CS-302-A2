#!/usr/bin/env bash
#SBATCH --account cs-302
#SBATCH --qos cs-302
#SBATCH --partition academic
#SBATCH --nodes 1
#SBATCH --ntasks-per-node 64
#SBATCH --cpus-per-task 1
#SBATCH --mem 32G
#SBATCH --time 1:30:00

module purge
module load gcc
module load openmpi

echo "STARTING AT $(date)"
echo ""

# Parameters
NROUNDS=10
SIZE=1000
NPROCS=2

# Compile
make || true

# Clean old outputs
rm -f model.csv model_seq.csv model_mpi.csv diff_output.txt

echo "Running sequential version: ./prog0 $NROUNDS $SIZE"
srun -n 1 ./prog0 "$NROUNDS" "$SIZE"

if [ ! -f model.csv ]; then
    echo "ERROR: prog0 did not produce model.csv"
    exit 1
fi

mv model.csv model_seq.csv

echo ""
echo "Running MPI version: srun -n $NPROCS ./progA $NROUNDS $SIZE"
srun -n "$NPROCS" ./progA "$NROUNDS" "$SIZE"

if [ ! -f model.csv ]; then
    echo "ERROR: progA did not produce model.csv"
    exit 1
fi

mv model.csv model_mpi.csv

echo ""
echo "Comparing model_seq.csv and model_mpi.csv..."

if cmp -s model_seq.csv model_mpi.csv; then
    echo "SUCCESS: CSV files are identical."
else
    echo "ERROR: CSV files are different."

    echo ""
    echo "Showing first differences:"
    diff -u model_seq.csv model_mpi.csv | head -n 100 > diff_output.txt
    cat diff_output.txt

    exit 1
fi

echo ""
echo "FINISHED AT $(date)"