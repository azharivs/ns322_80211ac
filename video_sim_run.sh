#!/bin/sh
simulationTime=20
nMpdus=64;
#rm -rf /home/aolnf/new-test/ns-allinone-3.22/ns-3.22/NF-results/output/*
defDir="/home/aolnf/new-test/ns-allinone-3.22/ns-3.22/NF-results"
logDir=$defDir"/NF-logfiles"
#YUVDir="/media/7C766F3A766EF474"



#if [ -d "$logDir" ] 
#then
#	rm -rf $logDir
#fi

outDir=$defDir"/output"


for nSta in 14
do
 	for dvp in 0.01
 	do
 		for dMax in 5
 		do
                        outVID=$logDir"/edf/nSta"$nSta"-dvp"$dvp"-dmax"$dMax"/video"
			outTrace=$logDir"/edf/nSta"$nSta"-dvp"$dvp"-dmax"$dMax"/out"
			#outYUV=$YUVDir"/YUV/edf-nSta"$nSta"-dvp"$dvp"-dmax"$dMax
			#if [ ! -d "$outYUV" ] 
			#then
			#	mkdir -p $outYUV
			#else  
			#	echo "directory "$outYUV" exist"
			#fi

			if [ ! -d "$outTrace" ]
			then
				mkdir -p $outTrace
			else  
				echo "directory "$outTrace" exist"
			fi

			if [ ! -d "$outVID" ] 
			then
				mkdir -p $outVID
			else  
				echo "directory "$outVID" exist"
			fi
			
 			cmdLine=$(echo "--nMpdus=$nMpdus --simulationTime=$simulationTime --nSta=$nSta --dMax=$dMax --dvp=$dvp")
                        logfile="logfiles/edfTA"_"nSta$nSta"_"dMax$dMax"_"dvp$dvp"
			export nSta outTrace outVID outYUV defDir outDir
 			./video_single_sim_run.sh $1 "$(echo $cmdLine)" $logfile
			
		done
	done 
done





