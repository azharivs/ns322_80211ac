#!/bin/sh

echo "-----------------------------------------------------------------------------------------------------------------------------------"
echo "Usage: "
echo "extract-all.sh NameOfLogFile NumberOfStations"
echo "Output file name will append .StaAgg to NameOfLogFile. "
echo "Will assume station MAC address starts from 00:00:00:00:00:01"
echo "Works for up to 9 stations"
echo "-----------------------------------------------------------------------------------------------------------------------------------"

./extract-channel-stat.sh $1

BaseMac="00:00:00:00:00:0"

for i in $(seq 1 $2) 
do
    Mac=$(echo $BaseMac$i)
    ./extract-sta-agg-stat.sh $1 $Mac
    ./extract-sta-q-stat.sh $1 $Mac
done

