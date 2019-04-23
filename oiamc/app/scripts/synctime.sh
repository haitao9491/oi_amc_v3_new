#!/bin/bash

APP_BASE_DIR=/application
PROJECT=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/version -S VersionInfo -A get -K project`
echo "PROJECT = $PROJECT"

while true;
do 
	outstr=`${APP_BASE_DIR}/bin/gettime`
	if [ "$?" == "0" ]; then
		sec=`echo $outstr | awk -F " " '{print $1}'`
		usec=`echo $outstr | awk -F " " '{print $2}'`

		echo sec: $sec  usec:$usec
        if [ "x$PROJECT" = "x030300" ]; then
            sec_hh=`echo $sec | awk '{printf("%02x",and(rshift($1,24), 0xff))}'`
            sec_hl=`echo $sec | awk '{printf("%02x",and(rshift($1,16), 0xff))}'`
            sec_lh=`echo $sec | awk '{printf("%02x",and(rshift($1,8), 0xff))}'`
            sec_ll=`echo $sec | awk '{printf("%02x",and(rshift($1,0), 0xff))}'`
            /myself/bin/reg w  0xf1000020 0x$sec_hh
            /myself/bin/reg w  0xf1000021 0x$sec_hl
            /myself/bin/reg w  0xf1000022 0x$sec_lh
            /myself/bin/reg w  0xf1000023 0x$sec_ll
            usec_hh=`echo $usec | awk '{printf("%02x",and(rshift($1,24), 0xff))}'`
            usec_hl=`echo $usec | awk '{printf("%02x",and(rshift($1,16), 0xff))}'`
            usec_lh=`echo $usec | awk '{printf("%02x",and(rshift($1,8), 0xff))}'`
            usec_ll=`echo $usec | awk '{printf("%02x",and(rshift($1,0), 0xff))}'`
            /myself/bin/reg w  0xf1000024 0x$usec_hh
            /myself/bin/reg w  0xf1000025 0x$usec_hl
            /myself/bin/reg w  0xf1000026 0x$usec_lh
            /myself/bin/reg w  0xf1000027 0x$usec_ll
            /myself/bin/reg w  0xf1000028 0x00
            /myself/bin/reg w  0xf1000028 0x01
        else
		    ${APP_BASE_DIR}/bin/oicapp cfg synctime $sec $usec
        fi

		sleep 600
	fi
done;
