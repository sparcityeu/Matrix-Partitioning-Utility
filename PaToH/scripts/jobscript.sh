#!/bin/bash
#SBATCH -J testing
#SBATCH -p defq # partition (queue)
#SBATCH -N 1 # number of nodes
#SBATCH -n 128  # number of cores
#SBATCH -t 0-4:00 # time (D-HH:MM)
#SBATCH -o slurm.%N.%j.out # STDOUT
#SBATCH -e slurm.%N.%j.err # STDERR
#SBATCH --exclusive

module load slurm

for matrixAdress in \
        /global/D1/homes/james/sparcity/suitesparse/mtx/Belcastro/human_gene1/human_gene1.mtx.gz\
        /global/D1/homes/james/sparcity/suitesparse/mtx/Belcastro/human_gene1/human_gene2.mtx.gz\
        /global/D1/homes/james/sparcity/suitesparse/mtx/Belcastro/mouse_gene/mouse_gene.mtx.gz\
        ; do
./eX3_partition_script.sh $matrixAdress
done
