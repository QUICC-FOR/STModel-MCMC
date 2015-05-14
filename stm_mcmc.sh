#!/bin/sh
#PBS -q default
#PBS -l walltime=48:00:00
#PBS -l nodes=14:ppn=20
#PBS -r n
#PBS -N stm_mcmc

module load gcc/4.9.2
module load openmpi/1.8.3

SRC=~/STModel-MCMC-0.9.1/bin
RUN2=~/STModel-MCMC-0.9.1/run2
RUN4=~/STModel-MCMC-0.9.1/run4

cd $RUN2/28731-ACE-SAC; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
cd $RUN2/18032-ABI-BAL; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
cd $RUN2/183319-PIN-BAN; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
cd $RUN2/28728-ACE-RUB; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &

cd $RUN2/19049-ULM-AME; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/19290-QUE-ALB; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/19489-BET-PAP; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/19481-BET-ALL; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/32931-FRA-AME; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &
cd $RUN2/183302-PIC-MAR; $SRC/stm2_mcmc -p inits.txt -t trans.txt -n 25 -i 10000 -b 5000 -c 20 -l 5 -v 2 2>log.txt &

# cd $RUN2/19049-ULM-AME; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
# cd $RUN2/19290-QUE-ALB; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
# cd $RUN2/19489-BET-PAP; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
# cd $RUN2/19481-BET-ALL; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
# cd $RUN2/32931-FRA-AME; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
# cd $RUN2/183302-PIC-MAR; $SRC/stm2_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
cd $RUN4/mcmc1; $SRC/stm4_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
cd $RUN4/mcmc2; $SRC/stm4_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
cd $RUN4/mcmc3; $SRC/stm4_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &
cd $RUN4/mcmc4; $SRC/stm4_mcmc -r resumeData.txt -t trans.txt -i 10000 2>log.txt &

wait
