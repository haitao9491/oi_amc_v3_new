#!/bin/bash

APPDIR=/application
NTPPID=/tmp/ntp.pid
BASEDIR=/myself

if [ -f "$NTPPID" ]; then
	touch $NTPPID
fi

serverip=

ntp_auto_find_serverip()
{

    rack=`$BASEDIR/bin/clvprtha | awk '{print $1; }'`
    shelf=`$BASEDIR/bin/clvprtha | awk '{print $2; }'`
	slot=`$BASEDIR/bin/clvprtha | awk '{print $3; }'`
    ga=0
    mm=`expr 10 + $rack`

    #auto find ntp serverip (mpcb ip) 
    for ip in $(seq 2)
    do
        ha=$slot
        nn=`expr $shelf \* 30 + \( $ha + 1 \) \* 5 + $ga`
        #echo "rack=$rak shelf=$shelf ha=$ha ga=$ga mm=$mm nn=$nn serverip=192.168.$mm.$nn"
        serverip="192.168.$mm.$nn"
        if ping -c 1 ${serverip} >& /dev/null
        then
            echo "${serverip}: OK !!!"
            break
        else
            echo "${serverip}: FAILED !!!"
        fi
    done

    echo "auot find serverip $serverip"
    killall ntpd
}

ntp_serverip()
{
    serverip=$1
    echo serverip $serverip
    killall ntpd
}

ntp_config()
{
	echo "Configure NTP ..."

echo > /etc/ntp.conf
cat << EOF > /etc/ntp.conf
#
# Automatically generated NTP configuration file (SERVER - Local Ref Clock).
#
# --- Do NOT modify! ---
#
restrict default nomodify notrap

restrict 127.0.0.1
restrict ::1

restrict 192.168.10.0 mask 255.255.255.0 nomodify notrap

server $serverip 

server 127.127.1.0
fudge 127.127.1.0 stratum 8

driftfile $BASEDIR/etc/ntp.drift
EOF
}

ntp_first()
{
	while true
	do
		/usr/sbin/ntpdate $serverip
		if [ $? == 0 ]; then
			echo "ntpdate $serverip ok ......"
			/usr/sbin/ntpd -c /etc/ntp.conf -g
            if [ -f ${APPDIR}/bin/synctime.sh ]; then
		    	${APPDIR}/bin/synctime.sh &
            fi
            rm $NTPPID
			return
		fi
		sleep 5
	done
}

ntp_start() 
{
	pid=$$
	echo "ntp_start pid=$pid"
	echo $pid > $NTPPID

	killall ntpd
	ntp_first
}

ntp_stop() 
{
    echo "ntp stop ......"
	killall ntpd
    if [ -f $NTPPID ];then
        pid=`cat $NTPPID`
        kill -0 $pid
        if [ "x$?" = "x0" ];then
            kill  $pid
        fi
    fi
}

case "$1" in
    --serverip)
        if [ $2 ]; then
            ntp_serverip $2
            ntp_config
            ntp_start
        fi
        ;;
	--start)
        ntp_auto_find_serverip
        ntp_config
		ntp_start
		;;
	--stop)
		ntp_stop
		;;
	--recovery)
		ntp_recovery
		;;
	*)
		echo "Usage: ntp.sh {--start|--stop} [debug]"
		;;
esac
