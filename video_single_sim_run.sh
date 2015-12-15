#!/bin/sh
#$1: path/main_file_name
#$2: sim_command_line_parameters
#$3: output_log_file
#example:
#$1=scratch/bss-universal-mpdu-aggregation 
#$2=--nMpdus=64 --nSta=7 --simulationTime=50 --dMax=10 
#$3=results/logfiles/pid.nSta7.dMax10
var="$2"
# Frome here .....

echo "$(echo "$1 --nSta=$nSta")"
./waf --run "$(echo $1 $var)" > results/$3

#echo 
cd $outDir
mv * $outTrace
cd $defDir
#cp st_fhd60_30.st fhd60_30.mp4 etmp4 $outVID 



#maxSta=`expr $nSta - 1`
#for j in `seq 0 $maxSta`
#do	
#	cp st_fhd60_30.st fhd60_30.mp4 etmp4 $outVID 
#	cd $outVID
#	./etmp4 -f -0 "$outTrace/sender-output$j" "$outTrace/receiver-output$j" st_fhd60_30.st fhd60_30.mp4 a01e_$j 
	#ffmpeg -i "$outVID/a01e_$j.mp4" "$outYUV/a01e_$j.yuv"
#        cp "$outVID/a01e_$j.mp4" $outYUV
        
#	rm -rf *
        
#	cd $defDir
        #./psnr 1920 1080 420 fhd60_30.yuv "$outYUV/a01e_$j.yuv" > "$outTrace/psnr_$j.txt" 
        #awk -f avg-psnr.awk "$outTrace/psnr_$j.txt" >>"$outTrace/avg-psnr-$nSta.txt"
	
#done



cd ..
cd results
./extract-all.sh $3 $nSta
/usr/local/MATLAB/R2010b/bin/matlab -nosplash -nodesktop -r "plot_results_func('$3',$nSta); quit;"
#rm -rf $3
cd ..

