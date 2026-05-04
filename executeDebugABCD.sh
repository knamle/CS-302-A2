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
SIZE=1000000
NPROCS=2
EXTRA="2048 2048"  # Extra args for progB and progC

# Compile
make || true

# Clean old outputs
rm -f model.csv model_mpi_*.csv diff_output_*.txt

# ---------- helper function ----------
run_and_check() {
    local NAME=$1
    local REF=$2
    local CMD=$3
    local OUTFILE="model_mpi_${NAME}.csv"

    echo "Running MPI version: $CMD"
    eval "$CMD"

    if [ ! -f model.csv ]; then
        echo "ERROR: $NAME did not produce model.csv"
        return 1
    fi
    mv model.csv "$OUTFILE"

    echo "Comparing $REF and $OUTFILE..."
    if cmp -s "$REF" "$OUTFILE"; then
        echo "SUCCESS: $NAME CSV matches $REF."
    else
        echo "ERROR: $NAME CSV differs from $REF."
        diff -u "$REF" "$OUTFILE" | head -n 100 > "diff_output_${NAME}.txt"
        cat "diff_output_${NAME}.txt"
    fi
    echo ""
}
# ---------- end helper ----------

# Run progA first — its output becomes the reference for the others
echo "Running reference: srun -n $NPROCS ./progA $NROUNDS $SIZE"
srun -n "$NPROCS" ./progA "$NROUNDS" "$SIZE"
if [ ! -f model.csv ]; then
    echo "ERROR: progA did not produce model.csv — cannot continue."
    exit 1
fi
mv model.csv model_mpi_progA.csv
echo ""

run_and_check "progB" "model_mpi_progA.csv" "srun -n $NPROCS ./progB $NROUNDS $SIZE $EXTRA"
run_and_check "progC" "model_mpi_progA.csv" "srun -n $NPROCS ./progC $NROUNDS $SIZE $EXTRA"
run_and_check "progD" "model_mpi_progA.csv" "srun -n $NPROCS ./progD $NROUNDS $SIZE"

echo "FINISHED AT $(date)"