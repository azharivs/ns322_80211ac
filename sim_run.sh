#!/bin/sh
simulationTime=100
nMpdus=64;

for nSta in 1
  do
  for dvp in 0.90
    do
    for dMax in 5 10 20
      do
      cmdLine=$(echo "--nMpdus=$nMpdus --simulationTime=$simulationTime --nSta=$nSta --dMax=$dMax --dvp=$dvp")
      logfile="logfiles/pid"_"nSta$nSta"_"dMax$dMax"_"dvp$dvp"
      export nSta
      ./single_sim_run.sh $1 "$(echo $cmdLine)" $logfile
      done
    done	
  done
