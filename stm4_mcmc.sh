#!/bin/sh
#PBS -q default
#PBS -l walltime=30:00:00
#PBS -l nodes=4:ppn=20
#PBS -r n
#PBS -N stm4_mcmc

module load gcc/4.9.2
module load openmpi/1.8.3

SRC=.~/STModel-MCMC
RUN=./run4

cd $RUN/mcmc1; $SRC/stm4_mcmc -p inits.txt -t trans.txt -n 1 -i 10000 -b 0 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN/mcmc2; $SRC/stm4_mcmc -p inits.txt -t trans.txt -n 1 -i 10000 -b 0 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN/mcmc3; $SRC/stm4_mcmc -p inits.txt -t trans.txt -n 1 -i 10000 -b 0 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN/mcmc4; $SRC/stm4_mcmc -p inits.txt -t trans.txt -n 1 -i 10000 -b 0 -c 20 -l 5 -v 2 2>log.txt &

wait