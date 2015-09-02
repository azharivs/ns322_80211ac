#!/bin/sh

echo "-----------------------------------------------------------------------------------------------------------------------------------"
echo "Usage: "
echo "extract-sta-q-stat.sh NameOfLogFile StationMacAddress "
echo "Output file name will append .StaQInfo to NameOfLogFile. "
echo "Column order: [Time] [Curr. Q Pkts] [Curr. Q MB] [Avg. Q Pkts] [Avg. Q MB] [Avg. Wait msec] [Arr. Rate pps] [Arr. Rate Mbps] [DVP] [Prob Empty] [Avg. Served Pkts] [Avg. Served Bytes (MB)] [Arrival Rate Surplus (pps)]"
echo "-----------------------------------------------------------------------------------------------------------------------------------"
 
grep PerStaQInfo::Update $1 | grep $2 | cut -d' ' -f1,7,9,12,14,17,20,22,25,29,31,33,35 > $1.StaQInfo.$2
