#!/bin/bash
#
# (C) Copyright 2018
# liye <ye.li@raycores.com>
#
# packinst.sh - A description goes here.
#

usage() {
	echo "usage: genimage.sh [options] <product>"
    echo "    --maplist           : pack for map list file."
}

if [ "`echo $0 | cut -c1`" = "/" ]; then
    PROGNAME=$0
else
    PROGNAME=`pwd`/$0
fi
PROGDIR=`dirname $PROGNAME`
option_maplist=$PROGDIR/maplist.cfg
while [ $# -gt 0 ]
do
	case "$1" in
        --maplist)
			shift
            option_maplist=$1
            ;;
	esac
	shift
done

CURRDIR=`pwd`
ROOTDIR=`dirname $0`/..
dest=`grep ^BOARDTYPE $ROOTDIR/Makefile | cut -d"=" -f2 | tr [:upper:] [:lower:]`
bdtype=$dest
#
# Install files
#
TOPBITPATH=/tftpboot/fpga.images/${bdtype%_*}
TILEPATH=/tftpboot/tileapp

grep "^map" $option_maplist | while read dummy project version fpgam fpgas tileapp cfg 
do
    if [ ! -z $project ]; then
        if echo $project | grep -Eq "^[0-9]*[0-9]$" ; then
            project=$project 
        else
            echo "Invalid format of project $project."
            exit 1
        fi
    fi
    if [ ! -z $version ]; then
        if echo $version | grep -Eq "^[0-9][0-9]*(\.[0-9][0-9]*){0,1}(-[1-9][0-9]*|)$" ; then
            version=V$version
        else
            echo "Invalid format of version $version."
            exit 1
        fi
    fi

    ODSTDIR=`mktemp -d /tmp/geninstXXXXXXXX`
    DSTDIR=$ODSTDIR

    PRODUCT=$cfg
    PRODCFG=$PROGDIR/cfg/$PRODUCT.cfg
    if [ ! -f $PRODCFG ]; then
        echo "Don't know which product to generate."
        exit 1
    fi

    if [ -z $version ]; then
        version=`get_definition version`
    fi
    versionhead=[VersionInfo]
    echo $versionhead > $DSTDIR/version
    echo "boardtype=$bdtype" >> $DSTDIR/version
    echo "project=$project" >> $DSTDIR/version
    echo "version=$version" >> $DSTDIR/version

    if [ "x$fpgam" = "xNA" ]; then
        echo "fpgam=$fpgam" >> $DSTDIR/version
    else
        top_name=`find $TOPBITPATH -name "*$fpgam*" | sort -k1 | tail -n 1`
        if [ "xtop_name" = "x" ]; then
            echo "Failed to fine FpgaFunctionM[$fpgam]."
            exit
        fi
        echo "fpgam=${top_name##*/}" >> $DSTDIR/version
        install $top_name $TOPBITPATH/top.bit 
    fi

    if [ "x$fpgas" = "xNA" ]; then
        echo "fpgas=$fpgas" >> $DSTDIR/version
    else
        top_name=`find $TOPBITPATH -name "*$fpgas*" | sort -k1 | tail -n 1`
        if [ "xtop_name" = "x" ]; then
            echo "Failed to fine FpgaFunctionS[$fpgas]."
            exit
        fi
        echo "fpgas=${top_name##*/}" >> $DSTDIR/version
        install $top_name $TOPBITPATH/top_1.bit
    fi

    if [ "x${tileapp}" = "xNA" ]; then
        echo "tileapp=NA" >> $DSTDIR/version
    else
        echo "tileapp=${tileapp}" >> $DSTDIR/version
        install $TILEPATH/${tileapp}.tbz $TILEPATH/tileapp.tbz
    fi

    grep "^install " $PRODCFG | while read dummy type srcpath file dst filename
do
    option_nostatic=1
    option_nocfgsample=1

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
        if [ "x$filename" == "x" -o "x$filename" == "x." ]; then
            install $srcpath/$file $DSTDIR/$dst
        else
            install $srcpath/$file $DSTDIR/$dst/$filename
        fi
    elif [ "$type" = "config" ]; then
        if [ "x$filename" == "x" -o "x$filename" == "x." ]; then
            if [ $option_nocfgsample -eq 0 ]; then
                install $srcpath/$file $DSTDIR/$dst/$file.sample
            else
                install $srcpath/$file $DSTDIR/$dst/$file
            fi
        else
            install $srcpath/$file $DSTDIR/$dst/$filename
        fi
    elif [ "$type" = "object" ]; then
        if [ "x$filename" == "x" -o "x$filename" == "x." ]; then
            install $srcpath/$file $DSTDIR/$dst
        else
            install $srcpath/$file $DSTDIR/$dst/$filename
        fi
    fi
done

cd $DSTDIR
date=`date +%Y%m%d`
vname=${cfg##*app}
vname=${vname##*-}
vfpganame=`grep ^fpgam $DSTDIR/version | cut -d"=" -f2`
vfpga=`echo ${vfpganame##*top_} | cut -d"." -f1`
if [ ! -f /tftpboot/$project ]; then
    mkdir -p /tftpboot/$project
fi
if [ ! -f /tftpboot/$project/$dest ]; then
    mkdir -p /tftpboot/$project/$dest
fi
if [ ! -f /tftpboot/$project/$dest/$date ]; then
    mkdir -p /tftpboot/$project/$dest/$date
fi

if [ "x${vfpga}" = "xNA" ]; then
   vfpga="" 
fi
if [ "x${vname}" != "x" ]; then
    vname="_${vname}"
fi
if [ "x${vfpga}" != "x" ]; then
    vfpga="_${vfpga}"
fi

OUTPUTDIR=/tftpboot/$project/$dest/$date/${version}${vname}${vfpga}
if [ ! -d ${OUTPUTDIR} ]; then
        mkdir -p ${OUTPUTDIR}
fi
echo "output ${OUTPUTDIR}"

tar czf ${OUTPUTDIR}/${PRODUCT}-${version}.tgz ./*  
mkfs.jffs2 -d $DSTDIR -e 131072 -q -b -o ${OUTPUTDIR}/Usr -n

cd ${OUTPUTDIR}
md5sum ${PRODUCT}-${version}.tgz > ${PRODUCT}-${version}.md5sum

cd $CURRDIR
rm -rf $DSTDIR
done
