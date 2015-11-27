#!/bin/sh
simulationTime=50
nMpdus=64;
rm -rf /home/aolnf/ns-allinone-3.22/ns-3.22/NF-results/output/*
#
#for nSta in 1 3 5 7 9 11 12 13 14
#for nSta in 5 6 7 8 9 10 11 12 13 14
#for nSta in 10 11 12 13 14
for nSta in 20
  do
  for dvp in 0.01
    do
    for dMax in 5
      do
      cmdLine=$(echo "--nMpdus=$nMpdus --simulationTime=$simulationTime --nSta=$nSta --dMax=$dMax --dvp=$dvp")
      logfile="logfiles/edf"_"nSta$nSta"_"dMax$dMax"_"dvp$dvp"
      export nSta
      ./single_sim_run.sh $1 "$(echo $cmdLine)" $logfile
      done
    done	
  done
