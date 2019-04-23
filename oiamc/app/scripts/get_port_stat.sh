#!/bin/sh

REG=/myself/bin/reg
base_addr=F1000000
port_sel=50
e1_sel=130
AU_PTR_H1=135  
AU_PTR_H2=136 
AU_PTR_0110_5=137 
AU_PTR_0110_NOR=138 
AU_PTR_1001_5=139 
AU_PTR_1001_NOR=13A 
V5_ERR_H=131
V5_ERR_L=132
V5_PTR=133
V5=134
TU_PTR_0110_5=13B 
TU_PTR_0110_NOR=13C 
TU_PTR_1001_5=13D 
TU_PTR_1001_NOR=13E 
E1_V=13F
SYNC_ERR_H=140
SYNC_ERR_L=141
E1_CHANGE=142
STD_E1=43
E1_SPEED=144
B_SEL=8D
B_VAL_H=8E
B_VAL_L=8F

conv_hex() {
	echo "obase=10; ibase=16; ${1}"|bc
	#printf "%d" $addr 
}

conv_dec() {
	echo "obase=16; ibase=10; ${1}"|bc
	#printf "%x" $1
}

conv_addr() {
	off_dec=`conv_hex $1`
	addr_dec=`conv_hex $base_addr`
	addr=`expr $addr_dec + $off_dec`
	conv_dec $addr
}

w_reg() {
	addr=`conv_addr $1`
	eval $REG w 0x$addr $2 >> /tmp/cfg.log  
}

get_val() {
	addr=`conv_addr $1`
	$REG r 0x$addr| awk -F '=' '{printf $2}'	
}

get_val_dec() {
	val_hex=`get_val $1 | awk '{print substr($0,8,3)}'`
	#conv_hex $val_hex
	printf "%d" 0x$val_hex
}

set_port_sel() {
	port=$1
	w_reg $port_sel $port
	usleep 1
}

set_e1_sel() {
	e1=$1
	w_reg $e1_sel $e1
	usleep 1
}

set_b_sel() {
	if [ $1 -eq 1 ]; then
		w_reg $B_SEL $b1_sel
	elif [ $1 -eq 2 ]; then
		w_reg $B_SEL $b2_sel
	elif [ $1 -eq 3 ]; then
		w_reg $B_SEL $b3_sel
	fi
	usleep 1	
}

get_port_stat() {

		if [ $1 -eq 1 ]; then
			b1_sel=0
			b2_sel=1
			b3_sel=2
			port=0
		elif [ $1 -eq 2 ]; then
			b1_sel=3
			b2_sel=4
			b3_sel=5
			port=16
		elif [ $1 -eq 3 ]; then
			b1_sel=6
			b2_sel=7
			b3_sel=8
			port=32
		elif [ $1 -eq 4 ]; then
			b1_sel=9
			b2_sel=A
			b3_sel=B
			port=48
		fi

	set_port_sel $port
	au_ptr_h1=`get_val_dec $AU_PTR_H1`
	au_ptr_h2=`get_val_dec $AU_PTR_H2`
	au_ptr_0110_5=`get_val_dec $AU_PTR_0110_5`
	au_ptr_0110_nor=`get_val_dec $AU_PTR_0110_NOR`
	au_ptr_1001_5=`get_val_dec $AU_PTR_1001_5`
	au_ptr_1001_nor=`get_val_dec $AU_PTR_1001_NOR`
	e1_change=`get_val_dec $E1_CHANGE`
	set_b_sel 1
	b1_val_h=`get_val_dec $B_VAL_H`
	b1_val_l=`get_val_dec $B_VAL_L`
	tmp=`expr $b1_val_h \* 256`
	b1_val=`expr $tmp + $b1_val_l`
	set_b_sel 2
	b2_val_h=`get_val_dec $B_VAL_H`
	b2_val_l=`get_val_dec $B_VAL_L`
	tmp=`expr $b2_val_h \* 256`
	b2_val=`expr $tmp + $b2_val_l`
	set_b_sel 3
	b3_val_h=`get_val_dec $B_VAL_H`
	b3_val_l=`get_val_dec $B_VAL_L`
	tmp=`expr $b3_val_h \* 256`
	b3_val=`expr $tmp + $b3_val_l`

	echo -e "\e[1;31m port    au_h1    au_h2    au_0110_5    au_0110_nor    au_1001_5    au_1001_nor    e1_change    B1    B2    B3\e[0m"
	printf " %-2d      %4d     %4d     %4d         %4d           %4d         %4d          %4d        %4d  %4d  %4d\r\n" $1 $au_ptr_h1   $au_ptr_h2   $au_ptr_0110_5   $au_ptr_0110_nor   $au_ptr_1001_5   $au_ptr_1001_nor $e1_change $b1_val $b2_val $b3_val
}

get_e1_stat() {
	echo -e "\e[1;31m e1    v5_err    v5_ptr    v5    tu_0110_5    tu_0110_nor    tu_1001_5    tu_1001_nor    e1_v    sync_err    E1_SPEED\e[0m"
	for i in `seq 0 62`
	do
		set_e1_sel $i
		e1=`expr $i + 1`
		v5_err_h=`get_val_dec $V5_ERR_H`
		v5_err_l=`get_val_dec $V5_ERR_L`
		tmp=`expr $v5_err_h \* 256`
		v5_err=`expr $tmp + $v5_err_l`
		v5_ptr=`get_val_dec $V5_PTR`
		v5=`get_val_dec $V5`
		tu_ptr_0110_5=`get_val_dec $TU_PTR_0110_5`
		tu_ptr_0110_nor=`get_val_dec $TU_PTR_0110_NOR`
		tu_ptr_1001_5=`get_val_dec $TU_PTR_1001_5`
		tu_ptr_1001_nor=`get_val_dec $TU_PTR_1001_NOR`
		e1_vaild=`get_val_dec $E1_V`
		sync_err_h=`get_val_dec $SYNC_ERR_H`
		sync_err_l=`get_val_dec $SYNC_ERR_L`
		tmp=`expr $sync_err_h \* 256`
		sync_err=`expr $tmp + $sync_err_l`
		e1_speed=`get_val_dec $E1_SPEED`
		printf " %-2d    %5d     %4d    %4d    %4d         %4d           %4d         %4d          %4d     %5d      %5d\r\n" $e1   $v5_err    $v5_ptr    $v5    $tu_ptr_0110_5    $tu_ptr_0110_nor    $tu_ptr_1001_5    $tu_ptr_1001_nor $e1_vaild $sync_err $e1_speed
	done
}

if [ $1 ]; then
	if [ $1 -gt 0 -a $1 -lt 5 ]; then
		get_port_stat $1
		get_e1_stat $1
		exit
	fi
fi

usage() {
	echo "./get_port_stat.sh <1~4>: get port stat"	
}
usage

