#!/bin/sh
#Columns: Time CurrIdleTime AvgIdleTime CurrBusyTime AvgBusyTime

echo "-----------------------------------------------------------------------------------------------------------------------------------"
echo "Usage: "
echo "extract-channel-stat.sh NameOfLogFile "
echo "Output file name will append .BssPhyMacStats to NameOfLogFile. "
echo "Column order: [Time] [Current Idle Time msec] [Average Idle Time msec] [Current Busy Time msec] [Average Busy Time msec]"
echo "-----------------------------------------------------------------------------------------------------------------------------------"
 

grep BssPhyMacStats::Update $1 | cut -d' ' -f1,4,7,10,13 > $1.BssPhyMacStats

