#!/bin/sh

echo "-----------------------------------------------------------------------------------------------------------------------------------"
echo "Usage: "
echo "extract-all.sh NameOfLogFile NumberOfStations"
echo "Output file name will append .StaAgg to NameOfLogFile. "
echo "Will assume station MAC address starts from 00:00:00:00:00:01"
echo "Works for up to 9 stations"
echo "-----------------------------------------------------------------------------------------------------------------------------------"

./extract-channel-stat.sh $1

BaseMac="00:00:00:00:00:"

for i in $(seq 1 $2) 
do
    if [ "$i" -le 15 ] ; then
	j=$(printf '0%x' $i)
    else
	j=$(printf '%x' $i)
    fi

    Mac=$(echo $BaseMac$j)

    ./extract-sta-agg-stat.sh $1 $Mac
    ./extract-sta-q-stat.sh $1 $Mac
    ./extract-sta-agg-ctrl-stat.sh $1 $Mac
    ./extract-sta-agg-per-rate-time-allowance-stat.sh $1 $Mac
done

