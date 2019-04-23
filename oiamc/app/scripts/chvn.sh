#!/bin/bash
#
# (C) Copyright 2017
# liye <ye.li@raycores.com>
#
# chvn.sh - A description goes here.
#

if [ "x$1" == "x" ]; then
    echo "Usage: chvn.sh {version} : change version with format x[.y]."
    exit
else
	if echo $1 | grep -Eq "^[0-9][0-9]*(\.[0-9][0-9]*){0,1}(-[1-9][0-9]*|)$" ; then
        version=$1
    else
        echo "Usage: chvn.sh {version} : change version with format x[.y]."
        exit
    fi
fi
DSTDIR=/application
CFGTOOL=/myself/bin/cfgtool
$CFGTOOL --logfile /var/log/cfgtool.log --file $DSTDIR/version -S VersionInfo -A set -K current --value $version

######ln -s /application/xx.xx/ applicaton/ #########
ln -s $DSTDIR/$version/apphook.sh $DSTDIR/
ln -s $DSTDIR/$version/bin/ $DSTDIR/
ln -s $DSTDIR/$version/etc/ $DSTDIR/
ln -s $DSTDIR/$version/driver/ $DSTDIR/
ln -s $DSTDIR/$version/fpga-images/ $DSTDIR/
ln -s $DSTDIR/$version/log/ $DSTDIR/
