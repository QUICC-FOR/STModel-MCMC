#!/bin/sh
#PBS -q default
#PBS -l walltime=168:00:00
#PBS -l nodes=9:ppn=20
#PBS -r n
#PBS -N stm_mcmc

module load gcc/4.9.2
module load openmpi/1.8.3

# prev version was 0.9.1
# current is 1.0.1
SRC=~/STModel-MCMC/bin
RUN2=~/STModel-MCMC/run2
RUN4=~/STModel-MCMC/run4

cd $RUN2/19049-ULM-AME; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/19462-FAG-GRA; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/32931-FRA-AME; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/183295-PIC-GLA; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/195773-POP-TRE; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &

cd $RUN4/mcmc1; $SRC/stm4_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN4/mcmc2; $SRC/stm4_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN4/mcmc3; $SRC/stm4_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN4/mcmc4; $SRC/stm4_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &

wait
