#!/bin/sh
#example: ./sim_run.sh scratch/bss-universal-mpdu-aggregation
simulationTime=20
nMpdus=64;

for nSta in 8 #1 2 3 4 5 6 7 8
  do
  for dvp in 0.01
    do
    for dMax in 5
      do
      cmdLine=$(echo "--nMpdus=$nMpdus --simulationTime=$simulationTime --nSta=$nSta --dMax=$dMax --dvp=$dvp")
      logfile="logfiles/edfstd"_"nSta$nSta"_"dMax$dMax"_"dvp$dvp"
      export nSta
      ./single_sim_run.sh $1 "$(echo $cmdLine)" $logfile
      done
    done	
  done
