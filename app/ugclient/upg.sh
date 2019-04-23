#!/bin/sh

BASE_ROOT=/myself
APP_ROOT=/application

UPG_ROOT=/tmp/upgrade


usage() {
	echo "Usage: `basename $0` upgrade <tftp server> <base version> <base file> <base check> <app version> <app file> <app check>"
	echo "       `basename $0` downgrade <app version>"
}

do_upgrade() {
	local host basever basefile basechks appver appfile appchks
	local sbasever sappver spappver
	local rc

	if [ ! $# -eq 7 ]; then
		return 253
	fi

	host=$1
	basever=$2
	basefile=$3
	basechks=$4
	appver=$5
	appfile=$6
	appchks=$7

	sbasever=`cat ${BASE_ROOT}/version | head -n 1`
	sappver=`cat ${APP_ROOT}/version | grep 'current' | cut -d'=' -f2`
	spappver=`cat ${APP_ROOT}/version | grep 'previous' | cut -d'=' -f2`

	[ ! -d $UPG_ROOT ] && mkdir -p $UPG_ROOT
	rm -rf $UPG_ROOT/*
	cd $UPG_ROOT
	
	### download ###
	if [ "x$basever" != "x0.0" ]; then
		tftp -g -r $basefile $host
		rc=$?
		[ $rc != 0 ] && return 1
		tftp -g -r $basechks $host
		rc=$?
		[ $rc != 0 ] && return 1
	fi
	if [ "x$appver" != "x0.0" ]; then
		tftp -g -r $appfile $host
		rc=$?
		[ $rc != 0 ] && return 4
		tftp -g -r $appchks $host
		rc=$?
		[ $rc != 0 ] && return 4
	fi

	### check ###
	if [ "x$basever" != "x0.0" ]; then
		md5sum -c $basechks
		rc=$?
		[ $rc != 0 ] && return 2
	fi
	if [ "x$appver" != "x0.0" ]; then
		md5sum -c $appchks
		rc=$?
		[ $rc != 0 ] && return 5
	fi

	### install ###
	if [ "x$appver" != "x0.0" ]; then
		mkdir -p ${APP_ROOT}/${appver}
		rm -rf ${APP_ROOT}/${appver}/*

		tar jxf $appfile -C ${APP_ROOT}/${appver}
		rc=$?
		[ $rc != 0 ] && { rm -rf ${APP_ROOT}/${appver}/*; return 6; }
	fi
	if [ "x$basever" != "x0.0" ]; then
		tar jxf $basefile -C $BASE_ROOT
		rc=$?
		[ $rc != 0 ] && return 3
	fi

	### cleanup ###
	if [ "x$appver" != "x0.0" ]; then
		rm -rf ${APP_ROOT}/${spappver}
		if [ -f ${APP_ROOT}/${sappver}/apphook.sh ]; then
			${APP_ROOT}/${sappver}/apphook.sh --stop
		fi
	fi

	### update version ###
	if [ "x$appver" != "x0.0" ]; then
		sed -i "/^previous/c\previous=${sappver}" ${APP_ROOT}/version
		sed -i "/^current/c\current=${appver}" ${APP_ROOT}/version
	fi
	if [ "x$basever" != "x0.0" ]; then
		echo $basever > ${BASE_ROOT}/version
	fi

	return 0
}

do_downgrade() {
	local appver
	local sappver spappver
	local rc

	if [ ! $# -eq 1 ]; then
		return 253
	fi

	appver=$1

	sappver=`cat ${APP_ROOT}/version | grep 'current' | cut -d'=' -f2`
	spappver=`cat ${APP_ROOT}/version | grep 'previous' | cut -d'=' -f2`

	### cleanup ###
	if [ -f ${APP_ROOT}/${sappver}/apphook.sh ]; then
		${APP_ROOT}/${sappver}/apphook.sh --stop
	fi

	### update version ###
	sed -i "/^previous/c\previous=${sappver}" ${APP_ROOT}/version
	sed -i "/^current/c\current=${appver}" ${APP_ROOT}/version

	return 0
}

# Main
if [ $# -ne 2 ]; then
	usage
	exit 253
fi

case "$1" in
	upgrade)
		shift
		do_upgrade $*
		rc=$?
		;;
	downgrade)
		shift
		do_downgrade $*
		rc=$?
		;;
	*)
		rc=253
		;;
esac

exit $rc

