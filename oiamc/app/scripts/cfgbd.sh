#########################################################################
# File Name: cfgbd.sh
# Author: ma6174
# mail: ma6174@163.com
# Created Time: 2017年09月26日 星期二 12时07分01秒
#########################################################################
#!/bin/bash

APPDIR=/application
OICAPP=${APPDIR}/bin/oicapp
PROJECT=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/version -S VersionInfo -A get -K project`
FPGAFUNCTIONM=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file /application/version -S VersionInfo -A get -K fpgam`
FPGAM=${FPGAFUNCTIONM%.*}
FPGAM=${FPGAM#*_}
FPGAM=${FPGAM%_*}

Usage() {
    echo "cfgbd.sh"
    echo "           --cfg                   "
    echo "                  devid            : cfg devid 0-255."
    echo "                  amcid            : cfg amcid (slotid)."
    echo "                  ex_ip {ip}       : cfg ex_ip."
    echo "                  gw {ip}          : cfg gateway."
    echo "                  ntp {ntpseverip} : cfg ntp severip."
    echo "                  dip {ip}         : cfg fpga dst ip."
    echo "                  sip {ip}         : cfg fpga src ip."
    echo "                  dport {port}     : cfg fpga dst port."
    echo "                  sport {port}     : cfg fpga src port."
    
    echo "           --show                   "
    echo "                  bdinfo           : show board info."
    echo "                  devid            : show devid 0-255."
    echo "                  amcid            : show amcid 0-255."
    echo "                  ex_ip            : show ex_ip."
    echo "                  gw               : show gw."
    echo "                  ntp              : show ntp serverip."
}

check_ip_vaild() {
    echo $1|grep "^[0-9]\{1,3\}\.\([0-9]\{1,3\}\.\)\{2\}[0-9]\{1,3\}$" > /dev/null;
    if [ $? -ne 0 ]
    then
        echo "ERROR $1 unvalid."
        return 1
    fi

    ipaddr=$1
    a=`echo $ipaddr|awk -F . '{print $1}'`  #以"."分隔，取出每个列的值
    b=`echo $ipaddr|awk -F . '{print $2}'`
    c=`echo $ipaddr|awk -F . '{print $3}'`
    d=`echo $ipaddr|awk -F . '{print $4}'`
    for num in $a $b $c $d
    do
        if [ $num -gt 255 ] || [ $num -lt 0 ]    #每个数值必须在0-255之间
        then
            echo "ERROR $ipaddr [$num] err."
            return 1
        fi
   done
   echo $ipaddr valid.
   return 0
}

cfg_func() {

    case "$1" in
        devid)
            devid=$2
            if [ "x$FPGAM" = "x325T_D70" ]; then
                /myself/bin/reg w 0xf1000009 $devid
                echo "set devid $devid."
            else
                ${APPDIR}/bin/oicapp cfg devid $devid
            fi
            if [ $? = 0 ]; then
                /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S devinfo -A set -K devid --value $devid 
            fi
            ;;
        amcid)
            amcid=$2
            echo "set amcid $amcid."
            if [[ $amcid < 0 ]] || [[ $amcid > 7 ]]; then
                echo "slot %d err, should 0~7."
                return -1
            fi
            if [[ "x$FPGAM" = "x325T_D70" ]]; then
                /myself/bin/reg w 0xf100000a $amcid
            fi
            if [[ "x$FPGAM" = "x325T_D81" ]]; then
                /myself/bin/reg w 0xf100000a $amcid
                /myself/bin/reg w 0xf100000b $amcid
            fi
            if [ $? = 0 ]; then
                /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S devinfo -A set -K amcid --value $amcid 
            fi
            ;;
        ex_ip)
            ex_ip=$2
            check_ip_vaild $ex_ip
            if [ $? != 0 ];then
                return 1 
            fi
	        mgtip=`/myself/bin/clvprtmgtip`
            vstr1=`echo ${ex_ip%.*}`
            vstr2=`echo ${mgtip%.*}`

            if [ "x$vstr1" == "x$vstr2" ]; then
                echo "[ERROR] mgtip:$mgtip ex_ip:$ex_ip in same segment net."
                return 1
            fi

            /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S EX_IP -A set -K ex_ip --value $ex_ip
            ifconfig eth0:1 $ex_ip 
            ;;
        gw)
            gw_old=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S GW -A get -K gw`
            route del default gw $gw_old
            gw_new=$2
            check_ip_vaild $gw_new
            if [ $? != 0 ] ; then
                return 1
            fi
            route add default gw $gw_new
            if [ $? != 0 ] ; then
                echo "failed set route gw $gw_new, use old-gw $gw_old."
                route add default gw $gw_old
                return 1
            fi
            /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S GW -A set -K gw --value $gw_new
            ;;

        ntp)
            serverip=$2
            check_ip_vaild $serverip
            if [ $? != 0 ] ; then
                return 1
            fi
            /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S NTP -A set -K serverip --value $serverip
            /application/bin/ntp.sh --stop
            sleep 2
            /application/bin/ntp.sh --serverip $serverip &
            ;;

        dip)
            dip=$2
            check_ip_vaild $dip
            if [ $? != 0 ] ; then
                return 1
            fi
            /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S FPGACFG -A set -K dip --value $dip
            $OICAPP cfg dip $dip
            echo "cfg dst-ip $dip complete."
            ;;

        sip)
            sip=$2
            check_ip_vaild $sip
            if [ $? != 0 ] ; then
                return 1
            fi
            /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S FPGACFG -A set -K sip --value $sip
            $OICAPP cfg sip $sip
            echo "cfg src-ip $sip complete."
            ;;

        dport)
            dport=$2
            if [ $dport -gt 65535 ]; then
                return 1
            fi
            /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S FPGACFG -A set -K dport --value $dport
            $OICAPP cfg dport $dport
            echo "cfg dst-port $dport complete."
            ;;

        sport)
            sport=$2
            if [ $sport -gt 65535 ]; then
                return 1
            fi
            /myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S FPGACFG -A set -K sport --value $sport
            $OICAPP cfg sport $sport
            echo "cfg src-port $sport complete."
            ;;
        *)
            return -1
            ;;
    esac
            return 0

}

show_func() {
    case "$1" in
        bdinfo)
            if [ "x$FPGAM" = "x325T_D23" ]; then
                $OICAPP show bdinfoex
            fi
            ;;

        devid)
            if [ "x$FPGAM" = "x325T_D70" ]; then
                devid=`/myself/bin/reg r 0xf1000009 | awk -F "=" '{printf $2}'`
                echo "show devid: $devid."
            else
                ${APPDIR}/bin/oicapp show bdinfoex | grep devid
            fi
            ;;
        amcid)
            if [ "x$FPGAM" = "x325T_D70" ]; then
                amcid=`/myself/bin/reg r 0xf100000a | awk -F "=" '{printf $2}'`
                echo "show amcid: $amcid."
            fi
            ;;
        ex_ip)
            ex_ip=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S EX_IP -A get -K ex_ip`
            echo "ex_ip : $ex_ip"
            ;;
        gw)
            gw=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S GW -A get -K gw`
            echo "gateway : $gw"
            ;;
        ntp)
            serverip=`/myself/bin/cfgtool --logfile /var/log/cfgtool.log --file ${APPDIR}/etc/bdinfo.cfg -S NTP -A get -K serverip`
            echo "ntp serverip : $serverip"
            ;;
        *)
            return -1
            ;;
    esac
            return 0
}

case "$1" in
    --cfg)
        if [[ $2 ]] && [[ $3 ]]; then
            cfg_func $2 $3
            exit
        fi
        Usage
        exit
        ;;
    --show)
        if [ $2 ]; then
            show_func $2
        fi
        if [ $? != 0 ];then
            Usage
            exit
        fi
        ;;
    *)
        Usage
        exit
        ;;
esac

