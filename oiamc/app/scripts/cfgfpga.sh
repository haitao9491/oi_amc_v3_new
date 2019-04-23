#!/bin/bash

rack=0 
shelf=0
slot=1
subslot=1
cardid=0

APP_FILE=${APP_BASE_DIR}/bin/oicapp
CFG_FILE=/etc/board.cfg

PROJECT=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/version -S VersionInfo -A get -K project`
FPGAFUNCTIONM=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/version -S VersionInfo -A get -K fpgam`
FPGAM=${FPGAFUNCTIONM%.*}
FPGAM=${FPGAM#*_}
FPGAM=${FPGAM%_*}
echo "PROJECT = $PROJECT FPGAM = $FPGAM"

if [[ "x$FPGAM" = "x325T_D70" ]] || [[ "x$FPGAM" = "x325T_D81" ]]; then
    devid=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APP_BASE_DIR}/etc/bdinfo.cfg -S devinfo -A get -K devid`
    echo "set devid $devid..."
    ${APP_BASE_DIR}/bin/cfgbd.sh --cfg devid  $devid

    amcid=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APP_BASE_DIR}/etc/bdinfo.cfg -S devinfo -A get -K amcid`
    echo "set slot $amcid..."
    ${APP_BASE_DIR}/bin/cfgbd.sh --cfg amcid $amcid

    /myself/bin/reg w 0xf1000033 4
    exit
fi

if [ -f ${CFG_FILE} ]; then
	rack=`grep "^rack" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
	shelf=`grep "^shelf" -r ${CFG_FILE} | awk -F '='  '{print $2}'`
	slot=`grep "^slot" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
	slot=`echo $slot | awk '{printf("%d", and($1, 0xf))}'`
	subslot=`grep "^subslot" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
fi
echo "rack: $rack shelf: $shelf slot: $slot subslot:$subslot"

SRCMAC=`cat /proc/cmdline | awk -F" " '{print $5}' | cut -d'=' -f2`
echo "srcmac: $SRCMAC"
${APP_FILE} cfg smac $SRCMAC

if [ "x$FPGAM" = "x325T_D13" ]; then
	${APP_FILE} cfg dmac 10:10:10:20:20:20 
	${APP_FILE} cfg slot $slot $subslot
fi

if [ "x$FPGAM" = "x325T_D23" ]; then
	let "cardid=$slot<<4"
	cardid=`expr $cardid + $subslot`
	D_MAC6=`printf "%x" $cardid`
	DSTMAC=10:10:10:20:20:$D_MAC6

    echo "slot $slot subslot $subslot"
	${APP_FILE} cfg slotex $slot $subslot

	echo "set devid..."
	devid=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APP_BASE_DIR}/etc/bdinfo.cfg -S devinfo -A get -K devid`
	${APP_BASE_DIR}/bin/cfgbd.sh --cfg devid $devid

	echo "dstmac: $DSTMAC"
	${APP_FILE} cfg dmac $DSTMAC

	dip=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APP_BASE_DIR}/etc/bdinfo.cfg -S FPGACFG -A get -K dip`
	sip=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APP_BASE_DIR}/etc/bdinfo.cfg -S FPGACFG -A get -K sip`
	dport=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APP_BASE_DIR}/etc/bdinfo.cfg -S FPGACFG -A get -K dport`
	sport=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APP_BASE_DIR}/etc/bdinfo.cfg -S FPGACFG -A get -K sport`
    echo "dst-ip $dip src-ip $sip dst-port $dport src-port $sport"
    ${APP_FILE} cfg dip $dip
    ${APP_FILE} cfg sip $sip
    ${APP_FILE} cfg dport $dport
    ${APP_FILE} cfg sport $sport
fi

