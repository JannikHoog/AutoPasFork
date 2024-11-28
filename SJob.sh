#!/bin/bash
#SBATCH -J MeasuringEnergyperFLOP
#SBATCH -o ./%x.%j.%N.out
#SBATCH -D ./build
#SBATCH --get-user-env
#SBATCH --clusters=cm2_tiny
#SBATCH --cpus-per-task=1
#SBATCH --export=NONE
#SBATCH --mail-user=jannik.hoog@gmx.de
#SBATCH --mail-type=end
#SBATCH --time=01:00:00

examples/md-flexible/md-flexible --yaml-filename examples/md-flexible/SpinodalDecomposition_equilibration.yaml
