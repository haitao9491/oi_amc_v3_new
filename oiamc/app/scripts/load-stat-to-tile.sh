#!/bin/sh

BDSCFG=/etc/board.cfg

svrip=$1
slot=$2
subslot=$3

if [ "x$svrip" = "x" -o "x$slot" = "x" -o "x$subslot" = "x" ]
then 
	exit -1
fi

echo "load-stat-to-tile.sh $svrip $slot $subslot"

ip=`ifconfig  eth0 |grep "inet addr" |cut  -f2 -d :|cut -f1 -d ' '|cut -f4 -d .`

rawdatafile=/tmp/stat.result
donefile=/tmp/stat.result.done

datafile=/tmp/stat_${ip}.result
df=/tmp/stat/stat_${ip}.result

echo $datafile

done=/tmp/stat_${ip}.result.done
dn=/tmp/stat/stat_${ip}.result.done

while true
do
	if [[ -f ${donefile} ]]; then
		mv ${donefile} ${done}
		if [[ -f ${rawdatafile} ]]; then
			mv ${rawdatafile} ${datafile}
			time=`date +"%Y-%m-%d-%H:%M:%S"`
			tftp -p -l ${datafile} -r /tmp/stat/stat_[$time]_${ip}-${slot}-${subslot}.result $svrip
			sleep 1
			tftp -p -l ${done} -r /tmp/stat/stat_[$time]_${ip}-${slot}-${subslot}.result.done $svrip
			sleep 1

			mv ${datafile} ${datafile}.bak
			rm -f  ${done}
		fi
	fi

	sleep 2
done
