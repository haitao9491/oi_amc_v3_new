#!/bin/sh
#
# (C) Copyright 2007, 2011
# Hu Chunlin <chunlin.hu@gmail.com>
#
# Generate JFFS2-based image shell script.
#

usage() {
	echo "usage: genimage.sh [options] <product>"
	echo "    --nocfgsample       : don't append .sample for configuration file."
	echo "    --nostatic          : convert app name to one w/o suffix of static."
	echo "    --versionfile       : generate version file in root directory."
	echo "    --versiondir        : generate version directory in root directory."
	echo "    --version {version} : product version with format Vx[.y]"
}

get_definition() {
	if [ ! -z $1 ]; then
		grep "^define [ ]*$1 " $PRODCFG | awk '{print $3;}'
	fi
}

ROOTDIR=`dirname $0`/..

#
# Global variables
#
option_nocfgsample=0
option_nostatic=0
option_versionfile=0
option_versiondir=0
option_version=
product=
version=
dest=`grep ^BOARDTYPE $ROOTDIR/Makefile | cut -d"=" -f2 | tr [:upper:] [:lower:]`

#
# Analyze command line arguments.
#
while [ $# -gt 0 ]
do
	case "$1" in
		--nocfgsample)
			option_nocfgsample=1
			;;
		--versionfile)
			option_versionfile=1
			;;
		--versiondir)
			option_versiondir=1
			;;
		--version)
			shift
			if [ $# -lt 1 ]; then
				echo "No version specified."
				usage
				exit 1
			fi
			option_version=$1
			;;
		--nostatic)
			option_nostatic=1
			;;
		*)
			product=$1
			;;
	esac
	shift
done

if [ ! -z $option_version ]; then
	if echo $option_version | grep -Eq "^V[0-9][0-9]*(\.[0-9][0-9]*){0,1}(-[1-9][0-9]*|)$" ; then
		version=$option_version
	else
		echo "Invalid format of version."
		usage
		exit 1
	fi
fi

if [ -z $product ]; then
	echo "Must specify a product."
	usage
	exit 1
fi

if [ "`echo $product | grep -e app -e base`" = "" ]; then
	echo "Only generate app/base image."
	usage
	exit 1
fi

CURRDIR=`pwd`

#
# Product information
#
if [ "`echo $0 | cut -c1`" = "/" ]; then
    PROGNAME=$0
else
    PROGNAME=`pwd`/$0
fi
PROGDIR=`dirname $PROGNAME`

PRODUCT=$product
PRODCFG=$PROGDIR/cfg/$PRODUCT.cfg
if [ ! -f $PRODCFG ]; then
	echo "Don't know which product to generate."
	exit 1
fi

PRODHOME=`get_definition home`
if [ "`echo $PRODHOME | cut -c1`" = "/" ]; then
	PRODUCTHOME=$PRODHOME
else
	PRODUCTHOME=$PROGDIR/cfg/$PRODHOME
fi
if [ ! -d $PRODUCTHOME ]; then
	echo "Product home $PRODUCTHOME is not a directory."
	exit 1
fi
cd $PRODUCTHOME

#
# Install files
#
ODSTDIR=`mktemp -d /tmp/geninstXXXXXXXX`
DSTDIR=$ODSTDIR
if [ -z $version ]; then
	version=`get_definition version`
fi

if [ $option_versionfile -eq 1 ]; then
	if [ $option_versiondir -eq 1 ];then
		v=`echo $version | cut -c2-`
		echo "current=$v" > $DSTDIR/version
	else
		echo $version | cut -c2- > $DSTDIR/version
	fi
fi

if [ $option_versiondir -eq 1 ];then
	DSTDIR=$ODSTDIR/`echo $version |cut -c2-`
fi
grep "^install " $PRODCFG | while read dummy type srcpath file dst
do
	install -d $DSTDIR/$dst
	if [ "$type" = "library" -o "$type" = "binary" ]; then
		if [ $option_nostatic -eq 0 ]; then
			dstfile=$file
		else
			dstfile=`echo $file | sed -e "s/\.static$//g"`
		fi
		if [ -z $CROSS_COMPILE ]; then
			install -s $srcpath/$file $DSTDIR/$dst/$dstfile
		else
			install -s --strip-program=${CROSS_COMPILE}strip $srcpath/$file $DSTDIR/$dst/$dstfile
		fi
	elif [ "$type" = "script" ]; then
		install $srcpath/$file $DSTDIR/$dst
	elif [ "$type" = "config" ]; then
		if [ $option_nocfgsample -eq 0 ]; then
			install $srcpath/$file $DSTDIR/$dst/$file.sample
		else
			install $srcpath/$file $DSTDIR/$dst/$file
		fi
	elif [ "$type" = "object" ]; then
		install $srcpath/$file $DSTDIR/$dst
	fi
done

DSTDIR=$ODSTDIR
if [ "`echo $product | grep base`" != "" ]; then
	mkfs.jffs2 -d $DSTDIR -e 131072 -q -b -o /tftpboot/$dest/Base -n
else
	mkfs.jffs2 -d $DSTDIR -e 131072 -q -b -o /tftpboot/$dest/Usr -n
fi

cd $PROGDIR
rm -rf $DSTDIR
