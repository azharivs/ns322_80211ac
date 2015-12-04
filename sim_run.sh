#!/bin/sh
simulationTime=20
nMpdus=64;
#rm -rf /home/aolnf/ns-allinone-3.22/ns-3.22/NF-results/output/*
#

for nSta in 1 2 3 4 5 6 7 8 9 10
  do
  for dvp in 0.01
    do
    for dMax in 5
      do
      cmdLine=$(echo "--nMpdus=$nMpdus --simulationTime=$simulationTime --nSta=$nSta --dMax=$dMax --dvp=$dvp")
      logfile="logfiles/deadline"_"nSta$nSta"_"dMax$dMax"_"dvp$dvp"
      export nSta
      ./single_sim_run.sh $1 "$(echo $cmdLine)" $logfile
      done
    done	
  done
