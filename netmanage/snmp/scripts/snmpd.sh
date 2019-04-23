#!/bin/bash

NETMGTDIR=/application/snmp

export LD_LIBRARY_PATH=/lib:/usr/lib:$NETMGTDIR/lib

$NETMGTDIR/sbin/snmpd -f -Lo -c $NETMGTDIR/etc/snmpd.conf > /dev/null 2>&1 &

