#!/bin/bash
#SBATCH -J MeasuringEnergyperFLOPCM4
#SBATCH -o ./%x.%j.%N.out
#SBATCH -D ./build
#SBATCH --get-user-env
#SBATCH --clusters=cm2_tiny
#SBATCH --cpus-per-task=28
#SBATCH --export=NONE
#SBATCH --mail-user=ge95piq@mytum.de
#SBATCH --mail-type=end
#SBATCH --time=20:00:00

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
examples/md-flexible/md-flexible --yaml-filename examples/md-flexible/SpinodalDecomposition_equilibration.yaml

