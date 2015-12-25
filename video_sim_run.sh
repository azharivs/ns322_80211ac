#!/bin/sh
simulationTime=65
nMpdus=64;

#please update the following address
#defDir="/home/aolnf/video/ns-allinone-3.22/ns-3.22/NF-results"
defDir="/home/azhari/app/ns-3-allinone/ns-3.22/NF-results"
logDir=$defDir"/NF-logfiles"
#YUVDir="/media/7C766F3A766EF474"
outDir=$defDir"/output"
res_logfiles="results/logfiles"

if [ ! -d "$res_logfiles" ]
then
	mkdir -p $res_logfiles
else  
	echo "directory "$res_logfiles" exist"
fi

if [ ! -d "$defDir" ] 
then
	mkdir -p $defDir
else  
	echo "directory "$defDir" exist"
fi

if [ ! -d "$outDir" ] 
then
	mkdir -p $outDir
else  
	echo "directory "$outDir" exist"
fi



#if [ -d "$logDir" ] 
#then
#	rm -rf $logDir
#fi


for nSta in 16
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
                        logfile="logfiles/ta"_"nSta$nSta"_"dMax$dMax"_"dvp$dvp"
			export nSta outTrace outVID outYUV defDir outDir
 			./video_single_sim_run.sh $1 "$(echo $cmdLine)" $logfile
			
		done
	done 
done





