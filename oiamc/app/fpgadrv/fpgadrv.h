/*
 * (C) Copyright 2015
 *  <www.raycores.com>
 *
 */

#ifndef	_FPGADRV_REG_H
#define _FPGADRV_REG_H

#include <linux/cdev.h>
#include "fpgamif_ioctl.h"

#define FPGADRV_MINOR               0	

struct ver_info {
	char ver;
	unsigned int data;
};

struct bd_cfg_info {
	char dstmac[6];
	char srcmac[6];
	int ethtype;
	char slot;
	char mask;  /* bit0: dstmac valid, bit1: srcmac valid, 
                  bit2: ethtype valid, bit3: slot valid */
};

struct scan_chan_ret {
	int  chan;       /* channel number */
	char scan_stat;  /* bit0: 0: scanning 1: scan complete bit1:0: scan failed 1: scan ok */
	char scan_cnt;   /* max=2 */
	char scan_ret1;  /* the first scan result */
	char scan_ret2;  /* the second scan result */
};

#define FPGADRV_OPTIC_PORT_CNT    4

struct board_port_info {
	/* sdh info */
	char optic_loss;               /* optic loss: 0: yes 1: no */
	char stm1_ok;                  /* stm1 frame: 0: no 1: yes */
	char e1_sync_cnt;              /* e1 frame count */
	int  ch_cnt_64k;               /* 64k channel count */
	unsigned int frame_cnt_64k;    /* 64k valid frame count per second */
	int  ch_cnt_2m;                /* 2M channel count */
	unsigned int frame_cnt_2m;     /* 2M valid frame count per second */
};

struct board_run_info {
	struct board_port_info bpi[FPGADRV_OPTIC_PORT_CNT];
	unsigned int ethbdw;           /* eth port bandwidth, bps */
};

/*
 * fpgadrv dev struct
 */
#define STARTUPED		1
typedef struct fpgadrv {
	struct cdev	     cdev;
	struct mutex  lock;

	int stat;
	struct class *fpgadrv_class;
	struct device *fpga_dev_node;

	int  num_fpga;
	unsigned long fpga_phys_addr;
	unsigned long fpga_phys_size;

	void __iomem  *vaddr_bk_fpga;

} fpgadrv_dev_t;

extern fpgadrv_dev_t *fpgadrv_devp;

#endif

