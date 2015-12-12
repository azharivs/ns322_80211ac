#!/bin/sh

echo "-----------------------------------------------------------------------------------------------------------------------------------"
echo "Usage: "
echo "extract-sta-agg-ctrl-stat.sh NameOfLogFile StationMacAddress "
echo "Output file name will append .StaAggCtrl to NameOfLogFile. "
echo "Column order: [Time] [current remaining time allowance] [new updated time allowance] [bitrate]"
echo "-----------------------------------------------------------------------------------------------------------------------------------"
 
grep -e "PerBitrateTimeAllowance::ResetTimeAllowance" $1 | grep $2 | cut -d' ' -f1,6,10,13 > $1.StaAggPerBitrateTa.$2

