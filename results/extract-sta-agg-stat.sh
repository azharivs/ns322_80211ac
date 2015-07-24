#!/bin/sh

echo "-----------------------------------------------------------------------------------------------------------------------------------"
echo "Usage: "
echo "extract-sta-agg-stat.sh NameOfLogFile StationMacAddress "
echo "Output file name will append .StaAgg to NameOfLogFile. "
echo "Column order: [Time] [Cur. A-MPDU Size in Bytes] [Cur. A-MPDU Size in Packets] [Data Rate] [A-MPDU Transmission Time in msec (no RTS/CTS/BACK)] "
echo "-----------------------------------------------------------------------------------------------------------------------------------"
 
grep MacLow::ForwardDown $1 | grep $2 | cut -d' ' -f1,7,10,13,16 > $1.StaAgg.$2
