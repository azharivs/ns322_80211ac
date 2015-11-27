#!/bin/sh
simulationTime=31
nMpdus=64;
rm -rf /home/aolnf/ns-allinone-3.22/ns-3.22/NF-results/output/*
defDir="/home/aolnf/ns-allinone-3.22/ns-3.22/NF-results"
logDir=$defDir"/NF-logfiles"
YUVDir="/media/01D11A6264B78140"

touch "$defDir/avg-psnr-$nSta.txt"


#YUVDir="/media/267E23D37E239A97"
if [ -d "$logDir" ] 
then
	rm -rf $logDir
fi

outDir=$defDir"/output"

 
#for nSta in 1 2 4 6 8 10 12 14 16 18 20
for nSta in 1
do
 	for dvp in 0.01
 	do
 		for dMax in 5
 		do
                        outVID=$logDir"/nSta"$nSta"-dvp"$dvp"-dmax"$dMax"/video"
			outTrace=$logDir"/nSta"$nSta"-dvp"$dvp"-dmax"$dMax"/out"
			outYUV=$YUVDir"/YUV/nSta"$nSta"-dvp"$dvp"-dmax"$dMax
			if [ ! -d "$outYUV" ] 
			then
				mkdir -p $outYUV
			else  
				echo "directory "$outYUV" exist"
			fi

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
                        logfile="logfiles/deadline"_"nSta$nSta"_"dMax$dMax"_"dvp$dvp"
			export nSta outTrace outVID outYUV defDir outDir
 			./video_single_sim_run.sh $1 "$(echo $cmdLine)" $logfile
			
		done
	done 
done

#touch "$logDir/avg-psnr.txt"
#for nSta in 2 4 6 
#do
#	awk -f avg-psnr.awk "$logDir/nSta-$nSta/out/avg-psnr_$nSta.txt" >> "$logDir/avg-psnr.txt"
#done



