#!/bin/sh
#$1: path/main_file_name
#$2: sim_command_line_parameters
#$3: output_log_file
#example:
#$1=scratch/bss-universal-mpdu-aggregation 
#$2=--nMpdus=64 --nSta=7 --simulationTime=50 --dMax=10 
#$3=results/logfiles/pid.nSta7.dMax10
var="$2"
echo $var
./waf --run "$(echo $1 $var)" > results/$3
cd results
./extract-all.sh $3 $nSta
matlab -nosplash -nodesktop -r "plot_results_func('$3',$nSta); "
rm -rf $3*
cd ..

