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
FPGA_CFG_APP=${APP_BASE_DIR}/bin/svcapp

init_fpga() {
	show_debug "Initialize FPGA."
	if [ -f ${FPGA_CFG_APP} ]; then
		${FPGA_CFG_APP} cfg bdstart
		if [ "$?" != "0" ]; then
			show_err "Failed to start FPGA."
			return 1
		fi

		return 0
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

	init_fpga
	if [ "$?" != "0" ]; then
		show_err "Failed to initialize FPGA."
		return 1
	fi

	set_indicator ok
	show_info "Confiture FPGA down..."
	return 0
}

start_application()
{
	show_info "Start application..."
	if [ -f ${APP_BASE_DIR}/bin/lkmp.sh ]; then
		${APP_BASE_DIR}/bin/lkmp.sh --start
	else 
		show_err "No such file ${APP_BASE_DIR}/bin/lkmp.sh."
	fi
}

appstart()
{
	start_cfg_fpga
	if [ "$?" != "0" ]; then
		set_indicator error
		return 1
	fi

	start_application
}

######---stop---#####
stop_application() {
	show_info "Stop Application..."
	if [ -f ${APP_BASE_DIR}/bin/lkmp.sh ]; then
		${APP_BASE_DIR}/bin/lkmp.sh --stop
	fi

	sleep 1
}

appstop() {
	stop_application
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
