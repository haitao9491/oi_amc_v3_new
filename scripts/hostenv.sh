#!/bin/bash

SuSERelFile=/etc/SuSE-release

distribution="Unknown"
if which lsb_release >/dev/null 2>/dev/null ; then
	distribution="`lsb_release -si`_`lsb_release -sr`"
elif [ -f $SuSERelFile ]; then
	version=`grep "^VERSION" ${SuSERelFile} | awk '{print $3;}'`
	patch=`grep "^PATCHLEVEL" ${SuSERelFile} | awk '{print $3;}'`
	distribution="SuSE_${version}.${patch}"
fi

echo "${distribution}_`uname -m`" | sed 's/ /_/g'
