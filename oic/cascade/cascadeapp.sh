#!/bin/bash
sleep 180
ROOTDIR=/application

LOG="--loglevel error --logsize 100M"
LOGDBG="--loglevel debug --logsize 100M"
LOGINFO="--loglevel info --logsize 100M"
LOGWARN="--loglevel warn --logsize 100M"

ulimit -c unlimited
rm -rf $ROOTDIR/log/*.log 2> /dev/null 
rm -rf $ROOTDIR/bin/cascadeapp.pid* 2> /dev/null
rm -rf /tmp/fpga* 2> /dev/null
mkdir /tmp/fpga0 2> /dev/null 
mkdir /tmp/fpga1 2> /dev/null
mkdir /tmp/fpga2 2> /dev/null
mkdir /tmp/fpga3 2> /dev/null
touch /tmp/fpga0/sgfp0.cfg /tmp/fpga0/sgfp1.cfg /tmp/fpga0/sgfp2.cfg /tmp/fpga0/sgfp3.cfg
touch /tmp/fpga1/sgfp0.cfg /tmp/fpga1/sgfp1.cfg /tmp/fpga1/sgfp2.cfg /tmp/fpga1/sgfp3.cfg
touch /tmp/fpga2/sgfp0.cfg /tmp/fpga2/sgfp1.cfg /tmp/fpga2/sgfp2.cfg /tmp/fpga2/sgfp3.cfg
touch /tmp/fpga3/sgfp0.cfg /tmp/fpga3/sgfp1.cfg /tmp/fpga3/sgfp2.cfg /tmp/fpga3/sgfp3.cfg
sleep 1

while true
do
	if [ ! -f $ROOTDIR/bin/cascadeapp.pid.1 ]; then
		$ROOTDIR/bin/cascadeapp -I 1  $LOGDBG --logfile $ROOTDIR/log/cascadeapp0.log --cfgfile $ROOTDIR/etc/cascade0.cfg
	fi
	if [ ! -f $ROOTDIR/bin/cascadeapp.pid.2 ]; then
		$ROOTDIR/bin/cascadeapp -I 2  $LOGDBG --logfile $ROOTDIR/log/cascadeapp1.log --cfgfile $ROOTDIR/etc/cascade1.cfg
	fi
	if [ ! -f $ROOTDIR/bin/cascadeapp.pid.3 ]; then
		$ROOTDIR/bin/cascadeapp -I 3  $LOGDBG --logfile $ROOTDIR/log/cascadeapp2.log --cfgfile $ROOTDIR/etc/cascade2.cfg
	fi
	if [ ! -f $ROOTDIR/bin/cascadeapp.pid.4 ]; then
		$ROOTDIR/bin/cascadeapp -I 4  $LOGDBG --logfile $ROOTDIR/log/cascadeapp3.log --cfgfile $ROOTDIR/etc/cascade3.cfg
	fi
	sleep 120
done
