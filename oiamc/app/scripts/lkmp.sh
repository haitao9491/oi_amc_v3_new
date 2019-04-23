#/bin/sh

APPDIR=/application
LKMPCFG=/application/etc/lkmp.cfg
LKMPAPP=/application/bin/psagent
SVCAPP=/application/bin/svcapp
RESTART_LKMP=0
NOKEY=1

dbg=0
show_debug() {
	if [ ${dbg} -eq 1 ]; then
		echo -e "\033[33m[DEBUG]: $1 \033[0m"
	fi
}

show_info() {
	echo -e "\033[32m[INFO]: $1\033[0m"
}

show_err() {
	echo -e "\033[31m[ERROR]: $1\033[0m" | tee -a ${APP_BASE_DIR}/log/err.log
}

devnum=0

app_gen_cfg() {
	CFG_FILE=/etc/board.cfg
	slot=`grep "^slot" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
    $APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S Psagent -A set -K slot --value $slot
    if [ "x$?" != "x0" ]; then
        return -1
    fi

	subslot=`grep "^subslot" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
    $APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S Psagent -A set -K subslot --value $subslot
    if [ "x$?" != "x0" ]; then
        return -1
    fi

	mgtip=`grep "^mgtip" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
	mgtip_last=`echo $mgtip | awk -F '.' '{print $4}'`
	mgtip_last=`expr $mgtip_last + 100`
	srcip=192.168.10.$mgtip_last
    $APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A set -K srcip --value $srcip
    if [ "x$?" != "x0" ]; then
        return -1
    fi

	devnum=`echo "$slot,$subslot" | awk -F',' '{print or(lshift($1,4), $2)}'`
    $APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A set -K devnum --value $devnum
    if [ "x$?" != "x0" ]; then
        return -1
    fi

	srcmac=`cat /proc/cmdline | awk -F" " '{print $5}' | cut -d'=' -f2`
	if [ "x$srcmac" == "x" ]; then
		mac_last=`printf %02x $mgtip_last`
		srcmac=aa:bb:cc:dd:ee:$mac_last
	fi
    $APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A set -K srcmac --value $srcmac
    if [ "x$?" != "x0" ]; then
        return -1
    fi

	selnum=`echo "$slot,$subslot" | awk -F',' '{print ($1-1)*16+($2-1)*2 }'`
    $APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A set -K selnum --value $selnum
    if [ "x$?" != "x0" ]; then
        return -1
    fi

    return 0
}

app_cfg_fpga() {
    srcmac=`$APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A get -K srcmac`
    if [ "x$?" != "x0" ]; then
        return -1
    fi

    srcip=`$APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A get -K srcip`
    if [ "x$?" != "x0" ]; then
        return -1
    fi

    dstip=`$APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A get -K dstip`
    if [ "x$?" != "x0" ]; then
        return -1
    fi

    srcport=`$APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A get -K srcport`
    if [ "x$?" != "x0" ]; then
        return -1
    fi

    dstport=`$APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A get -K dstport`
    if [ "x$?" != "x0" ]; then
        return -1
    fi

    dstport1=`$APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A get -K dstport1`
    if [ "x$?" != "x0" ]; then
        return -1
    fi

    devnum=`$APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A get -K devnum`
    if [ "x$?" != "x0" ]; then
        return -1
    fi

    selnum=`$APPDIR/bin/cfgtool --nopidfile --logfile /var/log/cfgtool.log --file $LKMPCFG -S FPGA_CFG -A get -K selnum`
    if [ "x$?" != "x0" ]; then
        return -1
    fi

	echo "start fpga configure."
	$SVCAPP cfg smac $srcmac
    if [ "x$?" != "x0" ]; then
        return -1
    fi
	$SVCAPP cfg sip $srcip
    if [ "x$?" != "x0" ]; then
        return -1
    fi
	$SVCAPP cfg dip $dstip
    if [ "x$?" != "x0" ]; then
        return -1
    fi
	$SVCAPP cfg sport $srcport
    if [ "x$?" != "x0" ]; then
        return -1
    fi
	$SVCAPP cfg dport1 $dstport
    if [ "x$?" != "x0" ]; then
        return -1
    fi
	$SVCAPP cfg dport2 $dstport1
    if [ "x$?" != "x0" ]; then
        return -1
    fi
	$SVCAPP cfg devid $devnum
    if [ "x$?" != "x0" ]; then
        return -1
    fi
	$SVCAPP cfg sel $selnum
    if [ "x$?" != "x0" ]; then
        return -1
    fi

	sleep 2
    return 0
}

lkmpstop() {
    while true;
    do
        if [ -f ${APPDIR}/bin/psagent.pid.0 ]; then
            pid=`cat ${APPDIR}/bin/psagent.pid.0`
            kill -0 $pid
            if [ "x$?" == "x0" ]; then
                kill $pid
                continue; 
			else
				rm -rf ${APPDIR}/bin/psagent.pid.0
            fi
        fi
        break
    done;
}

lkmpstart() {

	while true;
	do
		app_gen_cfg
		if [ "$?" != "0" ]; then
			sleep 1
			echo "app_gen_cfg failed."
			continue;
		fi
		app_cfg_fpga
		if [ "$?" == "0" ]; then
			break
		fi
		sleep 3
	done;

    lkmpstop	

	if [ -f $LKMPAPP ]; then
		echo "psagent start: "
		$LKMPAPP --loglevel info --logfile ${APPDIR}/log/psagent.log --logsize 1M --cfg /application/etc/lkmp.cfg 
	fi
}

case "$1" in
	--start)
        lkmpstart
		;;
	--stop)
		lkmpstop
		;;
	--recovery)
		lkmprecovery
		;;
	*)
		echo "Usage: lkmp.sh {--start|--stop} [debug]"
		;;
esac
