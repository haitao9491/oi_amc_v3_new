#/bin/sh

if [ -n BASEDIR ]; then
	export BASEDIR=/myself
fi

#VERSION=V-test
#export APP_BASE_DIR=/application/${VERSION}

#No command dirname
#export APP_BASE_DIR=$(cd `dirname $0`; pwd)
currdir=${0%/apphook.sh}
export APP_BASE_DIR=$(cd ${currdir}/ ; pwd)
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${APP_BASE_DIR}/lib

SILENCE=0
PROJECT=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/version -S VersionInfo -A get -K project`
FPGAFUNCTIONM=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/version -S VersionInfo -A get -K fpgam`
FPGAM=${FPGAFUNCTIONM%.*}
FPGAM=${FPGAM#*_}
FPGAM=${FPGAM%_*}
echo "PROJECT = $PROJECT FPGAM = $FPGAM"

if [ ! -d ${APP_BASE_DIR}/log ]; then
	mkdir -p ${APP_BASE_DIR}/log
fi

if [ ! -d ${APP_BASE_DIR}/conf ]; then
	mkdir -p ${APP_BASE_DIR}/conf
fi

#######----log----########
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

FPGA_BIN_FILE=${APP_BASE_DIR}/fpga-images/top.bit
FPGA_DRV=${APP_BASE_DIR}/driver/oicdrv.ko
FPGA_CFG_APP=${APP_BASE_DIR}/bin/oicapp
FPGA_CFG_SCRPT=${APP_BASE_DIR}/bin/cfgfpga.sh

init_fpga() {
	show_debug "Initialize FPGA."
	if [ -f ${APP_BASE_DIR}/bin/bc ]; then
		cp ${APP_BASE_DIR}/bin/bc /bin
	fi

	#1-do initialize FPGA. now use cfgfpga.sh as before
    if [ -f ${FPGA_CFG_SCRPT} ]; then
        show_debug "Config FPGA."
        ${FPGA_CFG_SCRPT}
        if [ "$?" != "0" ]; then
            show_err "Failed to config FPGA."
            return 1
        fi
    fi
    return 0
}

load_fpga_driver() {
	show_debug "Load FPGA driver."

	if [ -f ${FPGA_DRV} ]; then
        /sbin/rmmod ${FPGA_DRV} &>/dev/null
        if [[ "x$FPGAM" == "x325T_D70" ]] || [[ "x$FPGAM" == "x325T_D81" ]]; then
            /sbin/insmod  ${FPGA_DRV} gfpmode=1
        else
            /sbin/insmod  ${FPGA_DRV}
        fi
		return $?
	fi

	return 1
}

set_indicator() {
	if [ -f ${BASEDIR}/bin/clvmmcled ]; then
		case "$1" in
			ok)
				show_debug "Set indicator ok."
				${BASEDIR}/bin/clvledctl blink
				;;
			prepare)
				show_debug "set indicator prepare."
				${BASEDIR}/bin/clvmmcled red off
				${BASEDIR}/bin/clvledctl fblink
				;;
			error)
				show_debug "Set indicator error."
				${BASEDIR}/bin/clvledctl stop
				${BASEDIR}/bin/clvmmcled red on
				;;
			*)
				show_err "Error argument."
				;;
			esac
	fi
}

load_fpga()
{
	show_debug "Download FPGA image ${FPGA_BIN_FILE}"
	if [ -f ${BASEDIR}/bin/fpgaload ]; then

		if [ -f ${FPGA_BIN_FILE} ]; then
			${BASEDIR}/bin/fpgaload 0 ${FPGA_BIN_FILE}
			return $?
		else 
			show_err "No such file ${FPGA_BIN_FILE}."
		fi	
	fi

	return 1
}

start_cfg_fpga()
{
	show_info "Start to configure FPGA..."

	set_indicator prepare
	load_fpga
	if [ "$?" != "0" ]; then
		show_err "Failed to download FPGA."
		return 1
	fi

	load_fpga_driver
	if [ "$?" != "0" ]; then
		show_err "Failed to load FPGA driver."
		return 1
	fi

	init_fpga
	if [ "$?" != "0" ]; then
		show_err "Failed to initialize FPGA."
		return 1
	fi

	set_indicator ok
	show_info "Confiture FPGA down..."
	return 0
}

start_psagent()
{
	probetype="155M_CPOS"
	cardtype="AMC"
	tileip="192.168.10.13"
	svrport=7288
	if [ -f ${APP_BASE_DIR}/bin/psagent ]; then
		chmod a+x ${APP_BASE_DIR}/bin/psagent
		if [ "$SILENCE" == "1" ]; then
			${APP_BASE_DIR}/bin/psagent --loglevel info --logfile ${APP_BASE_DIR}/log/psagent.log --logsize 1 --probetype $probetype --cardtype $cardtype --svrip ${tileip} --svrport ${svrport} --slot ${slot} --subslot ${subslot} --silence
		else
			${APP_BASE_DIR}/bin/psagent --loglevel info --logfile ${APP_BASE_DIR}/log/psagent.log --logsize 1 --probetype $probetype --cardtype $cardtype --svrip ${tileip} --svrport ${svrport} --slot ${slot} --subslot ${subslot}
		fi
	else 
		show_err "No such file ${APP_BASE_DIR}/bin/psagent."
	fi
}

start_application()
{
	CFG_FILE=/etc/board.cfg
	rack=`grep "^rack" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
	shelf=`grep "^shelf" -r ${CFG_FILE} | awk -F '='  '{print $2}'`
	slot=`grep "^slot" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
	subslot=`grep "^subslot" -r ${CFG_FILE} | awk -F '=' '{print $2}'`
	boardtype=`grep "^boardtype" -r ${CFG_FILE} | awk -F '=' '{print $2}'`

	show_info "Start application..."

    if [ "x$FPGAM" == "x325T_D70" ]; then
        ${APP_BASE_DIR}/bin/cascadeapp.sh &
    fi
    if [ "x$PROJECT" == "x030100" ]; then
		start_psagent

        ex_ip=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/etc/bdinfo.cfg -S EX_IP -A get -K ex_ip`
        gw=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/etc/bdinfo.cfg -S GW -A get -K gw`

        show_info "EX_IP $ex_ip Gateway $gw"
        ifconfig eth0:1 $ex_ip
        route del default gw 192.168.1.1
        route add default gw $gw

        show_info "check port los..."
        ${APP_BASE_DIR}/bin/ch_los.sh &
    elif [ "x$PROJECT" == "x030300" ]; then
        amcid=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S devinfo -A get -K amcid` 
        echo "slot $amcid ..."
        ${APPDIR}/bin/cfgbd.sh --cfg  amcid $amcid
    elif [ "x$PROJECT" == "x010100" ] || [ "x$PROJECT" == "x010200" ]; then
		SILENCE=1
		start_psagent 
    elif [ "x$PROJECT" == "x010300" ]; then
		start_psagent
    fi
}

app_is_running()
{
    show_info "app_is_running...."
    if [ -f ${APPDIR}/bin/psagent.pid.0 ]; then
        pid=`cat ${APPDIR}/bin/psagent.pid.0`
        kill -0 $pid
        if [ "x$?" == "x0" ]; then
            kill $pid
		else
			rm -rf ${APPDIR}/bin/psagent.pid.0
        fi
    fi
	return 0
}

start_ntp() {
    if [ "x$FPGAM" = "x325T_D23" ]; then
        ${APP_BASE_DIR}/bin/ntp.sh --start &
    fi
    if [ "x$PROJECT" = "x030300" ]; then
        ntpserverip=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S NTP -A get -K serverip` 
        ${APP_BASE_DIR}/bin/ntp.sh --serverip $ntpserverip &
    fi
}

appstart()
{
	app_is_running
	if [ "$?" != "0" ]; then
		return 1
	fi

	start_cfg_fpga
	if [ "$?" != "0" ]; then
		set_indicator error
		return 1
	fi

    start_ntp
    if [ "$?" != "0" ]; then
        set_indicator error
        return 1
    fi

    start_application
}

######---stop---#####
stop_fpga() {
	show_info "Stop FPGA..."
	/sbin/rmmod  oicdrv.ko
}

stop_ntp() {
	show_info "Stop NTP..."
	${APP_BASE_DIR}/bin/ntp.sh --stop
}

stop_application() {
	show_info "Stop Application..."
	sleep 1
    if [ "x$PROJECT" == "x030100" ] || [ "x$PROJECT" == "x010100" ] || \
		[ "x$PROJECT" == "x010200" ] || [ "x$PROJECT" == "x010300" ] ; then
        killall psagent    
    fi
    if [ "x$FPGAM" == "x325T_D70" ]; then
        killall cascadeapp
        killall cascadeapp.sh
    fi
}

appstop() {
	stop_application
	stop_fpga
	stop_ntp
}

#####---ALL---######
if [  "$#" = "2" -a "$2" == "--debug" ]; then
	dbg=1
fi

case "$1" in
	--start)
		appstart
		;;
	--stop)
		appstop
		;;
	*)
		echo "Usage: apphook.sh {--start|--stop} [--debug]"
		;;
esac
