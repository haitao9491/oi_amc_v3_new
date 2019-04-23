#!/bin/sh

if [ "x$1" = "x--boot" ]; then
	bootmode=1
	shift
fi

export BASEDIR=/myself
export APPDIR=/application

if [ ! -d ${BASEDIR}/etc ]; then
    mkdir -p ${BASEDIR}/etc
fi

# Network configuration
[ -z $ip ] && ip=192.168.10.6
[ -z $netmask ] && netmask=255.255.255.0
[ -z $ip1 ] && ip1=192.168.1.50


start_hosts() {
	echo "Configure hosts ..."

	cat << EOF > /etc/hosts
127.0.0.1	localhost.localdomain	localhost
::1	localhost6.localdomain6	localhost6

$ip	me
EOF
}

start_ethernet_fp() {
	echo "Bring up eth1 ... ($ip1 $netmask )"
	/sbin/ifconfig eth1 $ip1 netmask $netmask
	/bin/sleep 1
}

start_ethernet() {
	mgtip=`cat /etc/board.cfg | grep ^mgtip | cut -d"=" -f2`

    echo "Bring up eth0 ... ($mgtip $netmask)"
    /sbin/ifconfig eth0 $mgtip netmask $netmask
	/bin/sleep 1
}

start_cloves() {
	CLOVESARG="flashled=1"
	echo "Installing kernel module [clovesdrv - $CLOVESARG] ..."
	/sbin/insmod $BASEDIR/obj/clovesdrv.ko $CLOVESARG
	/bin/usleep 100000
}

stop_cloves() {
	echo "Remove kernel module [clovesdrv] ..."
	/sbin/rmmod clovesdrv
}

start_board_info() {
    echo "Prepare board information ..."

	hainfo=`$BASEDIR/bin/clvprtha`

	boardtype=`cat /proc/cpuinfo | grep platform | cut -d" " -f2- | sed -e 's/ /_/g'`
	if [ "$hainfo" = "" ]; then
		rack=1
		shelf=1
		slot=7
		subslot=7
		mgtip=192.168.10.6
	else
		rack=`echo $hainfo | awk '{print $1; }'`
		shelf=`echo $hainfo | awk '{print $2; }'`
		slot=`echo $hainfo | awk '{print $3; }'`
		subslot=`echo $hainfo | awk '{print $4; }'`
		mgtip=`$BASEDIR/bin/clvprtmgtip`
	fi

	cat << EOF > /etc/board.cfg
boardtype=$boardtype
rack=$rack
shelf=$shelf
slot=$slot
subslot=$subslot
mgtip=$mgtip
EOF
    cp /etc/board.cfg $BASEDIR/etc
}

stop_board_info() {
	rm -rf /etc/board.cfg
}

start_ntp() {
	echo "Configure NTP ..."
    if [ ! -f $BASEDIR/etc ]; then
        mkdir -p $BASEDIR/etc
    fi

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

server 127.127.1.0
fudge 127.127.1.0 stratum 10

driftfile $BASEDIR/etc/ntp.drift
EOF

	if [ ! -f $BASEDIR/etc/ntp.drift ]; then
		rm -f $BASEDIR/etc/ntp.drift
		echo 0 > $BASEDIR/etc/ntp.drift
	fi

	/usr/sbin/ntpd -c /etc/ntp.conf -g
}

stop_ntp() {
	echo "Stopping NTP service ..."

	/bin/killall ntpd
	/bin/sleep 1
	rm -rf /etc/ntp.conf
}

start_upgrade() {
	echo "Starting Upgrade server ..."

	type=`cat /etc/board.cfg | grep ^boardtype | cut -d"=" -f2`
	slot=`cat /etc/board.cfg | grep ^slot | cut -d"=" -f2`
	subslot=`cat /etc/board.cfg | grep ^subslot | cut -d"=" -f2`
	ctrladdr=`cat /etc/board.cfg | grep ^mgtip | cut -d"=" -f2`

	mkdir -p /tmp/upgrade
	cp $BASEDIR/bin/upgcli /tmp/upgrade/
	cp $BASEDIR/bin/upg.sh /tmp/upgrade/

	cd /tmp/upgrade
	./upgcli --loglevel debug --logsize 1M --slot $slot --subslot $subslot --type $type --ctrladdr $ctrladdr
	cd - >/dev/null
}

stop_upgrade() {
	echo "Stopping Upgrade server ..."

	/bin/killall upgcli
	/bin/sleep 1
	rm -rf /tmp/upgrade
}

start_application() {
	echo "Cfg pll..."
    ${BASEDIR}/bin/clvpllctl

	echo "Starting application ..."

	if [ -f $APPDIR/version ]; then
        if [ -x $APPDIR/apphook.sh ]; then
            $APPDIR/apphook.sh --start
        fi
	fi
}

stop_application() {
	echo "Stopping application ..."

	if [ -f $APPDIR/version ]; then
        if [ -x $APPDIR/apphook.sh ]; then
            $APPDIR/apphook.sh --stop
        fi
	fi
}

start() {
	start_ethernet_fp
	start_cloves
	start_board_info
	start_hosts
	start_ethernet
	start_ntp
	start_upgrade
	start_application
}

stop() {
	stop_application
	stop_upgrade
	stop_ntp
	stop_board_info
	stop_cloves
}

case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	*)
		echo "Usage: hook.sh {start|stop}"
		;;
esac

