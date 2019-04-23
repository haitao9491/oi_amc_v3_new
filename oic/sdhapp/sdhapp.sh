#!/bin/bash
#
# (C) Copyright 2018
# liye <ye.li@raycores.com>
#
# sdhapp.sh - A description goes here.
#

CFG=/application/etc/sdhapp.cfg
if [ ! -s $CFG ]; then
    echo "$CFG is unexist."
    exit
fi
CFGTOOL=/application/bin/cfgtool
if [ ! -f $CFGTOOL ]; then
    echo "$CFGTOOL is unexist."
    exit
fi
REG=/myself/bin/reg

usage()
{
    echo "sdhapp.sh <10G|2.5G|622M|155M> [src-port 0-xx] [src-au4 0-xx] <en> [dst-sel 0-xx] [dst-au4 0-3] "
    echo "sdhapp.sh <10G|2.5G|622M|155M> [src-port 0-xx] [src-au4 0-xx] <dis> "
    echo "sdhapp.sh <clr> [fpga0|fpga1|fpga2|fpga3]   "
}

wreg_valdec()
{
    if [[ "x$1" = "x" ]] || [[ "x$2" = "x" ]]; then
        return 1
    fi
    off=$1
    val=$2

    base_addr_dec=`echo "obase=10; ibase=16; $base_addr"|bc`
    off_dec=`echo "obase=10; ibase=16; $off"|bc`
    addr_dec=`expr $off_dec + $base_addr_dec`
	addr_hex=`echo "obase=16; ibase=10; ${addr_dec}"|bc`

    $REG w 0x$addr_hex $val > /dev/null
    return $? 
}

wreg_valhex()
{
    if [[ "x$1" = "x" ]] || [[ "x$2" = "x" ]]; then
        return 1
    fi
    off=$1
    val=0x$2

    base_addr_dec=`echo "obase=10; ibase=16; $base_addr"|bc`
    off_dec=`echo "obase=10; ibase=16; $off"|bc`
    addr_dec=`expr $off_dec + $base_addr_dec`
	addr_hex=`echo "obase=16; ibase=10; ${addr_dec}"|bc`

    $REG w 0x$addr_hex $val > /dev/null
    return $?
}

if [ "$#" -le 1 ]; then
    usage
    exit
fi

### reg offset
mif_list_cfg_en=41
mif_list_cfg_ch=42
mif_list_cfg_info1=43
mif_list_cfg_info0=44
mif_list_cfg_type=45
reg_sel=110
reg_val3=111
reg_val2=112
reg_val1=113
reg_val0=114
reg_cfg_en=115

fpgaid=
base_addr=0
sw_en_port0=0
sw_en_port1=0
sw_en_port2=0
sw_en_port3=0

select_baseaddr()
{
    case "$1" in
        fpga0)
            fpgaid=0
            base_addr=F1002000
            sw_en_port0=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga0_0`
            sw_en_port1=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga0_1`
            sw_en_port2=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga0_2`
            sw_en_port3=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga0_3`
            ;;
        fpga1)
            fpgaid=1
            base_addr=F1002800
            sw_en_port0=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga1_0`
            sw_en_port1=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga1_1`
            sw_en_port2=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga1_2`
            sw_en_port3=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga1_3`
            ;;
        fpga2)
            fpgaid=2
            base_addr=F1003000
            sw_en_port0=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga2_0`
            sw_en_port1=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga2_1`
            sw_en_port2=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga2_2`
            sw_en_port3=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga2_3`
            ;;
        fpga3)
            fpgaid=3
            base_addr=F1003800
            sw_en_port0=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga3_0`
            sw_en_port1=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga3_1`
            sw_en_port2=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga3_2`
            sw_en_port3=`$CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A get -K fpga3_3`
            ;;
        *)
            usage
            exit
            ;;
    esac
 
}

if [ "x$1" = "xclr" ]; then
    select_baseaddr $2
   
    echo "clear fpga${fpgaid} sw ..."
    wreg_valdec $reg_sel 0  
    wreg_valdec $reg_val3 0 
    wreg_valdec $reg_val2 0 
    wreg_valdec $reg_val1 0 
    wreg_valdec $reg_val0 0 
    wreg_valdec $reg_cfg_en 0 
    wreg_valdec $reg_cfg_en 1 

    wreg_valdec $reg_sel 1 
    wreg_valdec $reg_val3 0 
    wreg_valdec $reg_val2 0 
    wreg_valdec $reg_val1 0 
    wreg_valdec $reg_val0 0 
    wreg_valdec $reg_cfg_en 0 
    wreg_valdec $reg_cfg_en 1 

    wreg_valdec $reg_sel 2 
    wreg_valdec $reg_val3 0 
    wreg_valdec $reg_val2 0 
    wreg_valdec $reg_val1 0 
    wreg_valdec $reg_val0 0 
    wreg_valdec $reg_cfg_en 0 
    wreg_valdec $reg_cfg_en 1 

    wreg_valdec $reg_sel 3 
    wreg_valdec $reg_val3 0 
    wreg_valdec $reg_val2 0 
    wreg_valdec $reg_val1 0 
    wreg_valdec $reg_val0 0 
    wreg_valdec $reg_cfg_en 0 
    wreg_valdec $reg_cfg_en 1 


    for((i=0;i<4;i++))
    do
        $CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A set -K fpga${fpgaid}_${i} --value 0
    done
    exit
fi

speed=$1
    case "$1" in
        10G)
            au4num=64
            ;;
        2.5G)
            au4num=16
            ;;
        622M)
            au4num=4
            ;;
        155M)
            au4num=0
            ;;
        *)
            usage
            exit
            ;;
    esac

hport=$2
if [ "x$hport" = "x" ]; then
    echo "src port null."
    usage
    exit
fi

hau4=$3
if [ "x$hau4" = "x" ]; then
    echo "src au4 null."
    usage
    exit
fi

en=$4
if [[ "x$en" != "xen" ]] && [[ "x$en" != "xdis" ]]; then
    echo "unkown en $en."
    usage
    exit
fi

if [ $hport -le 1 ]; then
    select_baseaddr fpga0
elif [[ $hport -ge 2 ]] && [[ $hport -le 3 ]]; then
    select_baseaddr fpga1
    ch_en=`echo "obase=10; ibase=16; $sw_en_port1"|bc`
elif [[ $hport -ge 4 ]] && [[ $hport -le 5 ]]; then
    select_baseaddr fpga2
    ch_en=`echo "obase=10; ibase=16; $sw_en_port2"|bc`
elif [[ $hport -ge 6 ]] && [[ $hport -le 7 ]]; then
    select_baseaddr fpga3
    ch_en=`echo "obase=10; ibase=16; $sw_en_port3"|bc`
fi

tmpc=`echo $((hport%2))`
if [ $tmpc -ne 0 ]; then
    tmpau4=`expr $au4num + $hau4`
else
    tmpau4=$hau4
fi

ch_group=`echo $((tmpau4/32))`
case "$ch_group" in
    0)
        ch_en=`echo "obase=10; ibase=16; $sw_en_port0"|bc`
        ;;
    1)
        ch_en=`echo "obase=10; ibase=16; $sw_en_port1"|bc`
        ;;
    2)
        ch_en=`echo "obase=10; ibase=16; $sw_en_port2"|bc`
        ;;
    3)
        ch_en=`echo "obase=10; ibase=16; $sw_en_port3"|bc`
        ;;
    *)
        ;;
esac
ch_p=`echo $((tmpau4%32))`

### dis sw rule
if [ "x$en" = "xdis" ]; then
    wreg_valdec $reg_sel $ch_group
    tmp1=`printf "%d" $((1<<ch_p))`
    tmp2=`printf "%X" $((~tmp1))`
    tmp3=`echo ${tmp2:0-8:8}`
    tmp4=`echo "obase=10; ibase=16; $tmp3"|bc`
    tmp5=`printf "%d" $((tmp4&ch_en))`

    tmp6=`printf "%d" $((tmp5>>24))`
    ch_val3=`printf "%02X" $((tmp6&0xff))`
    tmp7=`printf "%d" $((tmp5>>16))`
    ch_val2=`printf "%02X" $((tmp7&0xff))`
    tmp8=`printf "%d" $((tmp5>>8))`
    ch_val1=`printf "%02X" $((tmp8&0xff))`
    ch_val0=`printf "%02X" $((tmp5&0xff))`
    
    wreg_valhex $reg_val3 $ch_val3
    wreg_valhex $reg_val2 $ch_val2
    wreg_valhex $reg_val1 $ch_val1
    wreg_valhex $reg_val0 $ch_val0
    wreg_valhex $reg_cfg_en 0
    wreg_valhex $reg_cfg_en 1

	tmp10=`printf "%x" $((tmp5))`
    $CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A set -K fpga${fpgaid}_${ch_group} --value ${tmp10}

    exit
fi

sel=$5
if [ "x$sel" = "x" ];then
    echo "sel is null."
    usage
    exit
fi

lau4=$6
if [ "x$lau4" = "x" ];then
    echo "dst au4 null."
    usage
    exit
fi

hportnum=`printf "%d" $((hport%2))`
tmp1=`printf "%d" $((hportnum<<6))`
tmp2=`printf "%d" $((tmp1|hau4))`
wreg_valdec $mif_list_cfg_ch $tmp2
wreg_valdec $mif_list_cfg_info0 $sel
wreg_valdec $mif_list_cfg_info1 $lau4
wreg_valdec $mif_list_cfg_type 1
wreg_valdec $mif_list_cfg_en 0
wreg_valdec $mif_list_cfg_en 1

if [ "x$en" = "xen" ]; then
    wreg_valdec $reg_sel $ch_group
    tmp1=`printf "%d" $((1<<ch_p))`
    tmp2=`printf "%d" $((tmp1|ch_en))`
    
    tmp6=`printf "%d" $((tmp2>>24))`
    ch_val3=`printf "%02X" $((tmp6&0xff))`
    tmp7=`printf "%d" $((tmp2>>16))`
    ch_val2=`printf "%02X" $((tmp7&0xff))`
    tmp8=`printf "%d" $((tmp2>>8))`
    ch_val1=`printf "%02X" $((tmp8&0xff))`
    tmp9=`printf "%d" $((tmp2>>0))`
    ch_val0=`printf "%02X" $((tmp9&0xff))`
    
    echo "debug] t1] $tmp1 t2] $tmp2 3] $ch_val3 2] $ch_val2 1] $ch_val1 0] $ch_val0"
    
    wreg_valhex $reg_val3 $ch_val3
    wreg_valhex $reg_val2 $ch_val2
    wreg_valhex $reg_val1 $ch_val1
    wreg_valhex $reg_val0 $ch_val0

    wreg_valhex $reg_cfg_en 0
    wreg_valhex $reg_cfg_en 1
    
    tmp10=`printf "%X" $((tmp2))`
    $CFGTOOL --nopidfile --logfile /tmp/cfgtool.log --file $CFG -S SWPORT  -A set -K fpga${fpgaid}_${ch_group} --value ${tmp10}
fi

