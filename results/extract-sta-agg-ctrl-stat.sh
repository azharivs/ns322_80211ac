#!/bin/sh

echo "-----------------------------------------------------------------------------------------------------------------------------------"
echo "Usage: "
echo "extract-sta-agg-ctrl-stat.sh NameOfLogFile StationMacAddress "
echo "Output file name will append .StaAggCtrl to NameOfLogFile. "
echo "Column order: [Time] [error] [control signal] [cur time allowance msec] [new time allowance msec]"
echo "-----------------------------------------------------------------------------------------------------------------------------------"
 
grep AggregationController $1 | grep $2 | cut -d' ' -f1,6,8,10,13 > $1.StaAggCtrl.$2
