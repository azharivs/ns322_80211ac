#!/bin/sh

echo "-----------------------------------------------------------------------------------------------------------------------------------"
echo "Usage: "
echo "extract-sta-agg-ctrl-stat.sh NameOfLogFile StationMacAddress "
echo "Output file name will append .StaAggCtrl to NameOfLogFile. "
echo "Column order: [Time] [error] [control signal] [cur time allowance msec] [new time allowance msec] [integral] [derivative] [reference] [adjust] [total time allowance] [thrHigh] [thrLow]"
echo "-----------------------------------------------------------------------------------------------------------------------------------"
 
grep AggregationController $1 | grep $2 | cut -d' ' -f1,6,8,10,13,20,22,24,26,28,30,32 > $1.StaAggCtrl.$2

#not needed, will handle it above
#grep AggregationController $1 | grep "(PidControllerWithThresholds)" | grep $2 | cut -d' ' -f1,6,8,10,13,20,22,24,26,28 > $1.StaAggCtrl.$2
