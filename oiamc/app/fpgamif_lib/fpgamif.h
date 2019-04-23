/*
 * (C) Copyright 2015
 *  <www.raycores.com>
 *
 */

#ifndef	_ESN_FPGA_MIF_H
#define _ESN_FPGA_MIF_H

struct ver_info {
	char ver;
	unsigned int data;
};

struct bd_cfg_info {
	char dstmac[6];
	char srcmac[6];
	int  ethtype;
	char slot;
	char mask;  /* bit0: dstmac valid, bit1: srcmac valid, 
                  bit2: ethtype valid, bit3: slot valid */
};

struct scan_chan_ret {
	int  chan;       /* channel number pcmid: bit15-bit5, timeslot bit4-0 */
	char scan_stat;  /* bit0: 0: scanning 1: scan complete bit1:0: scan failed 1: scan ok */
	char scan_cnt;   /* scan link count, max=2 */
	char scan_ret1;  /* the first link scan result */
	char scan_ret2;  /* the second link scan result */
};

#define ESNAPP_OPTIC_PORT_CNT    4

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
	struct board_port_info bpi[ESNAPP_OPTIC_PORT_CNT];
	unsigned int ethbdw;           /* eth port bandwidth, bps */
};

#if defined(__cplusplus)
extern "C" {
#endif

void *fpgamif_open(void);
/*start-up fpga.*/
int fpgamif_start_up(void *hd);
/* get fpga version info */
int fpgamif_get_version(void *hd, struct ver_info *ver);
/* get fpga start status */
int fpgamif_get_board_start(void *hd, int *start);
/* get fpga board config info */
int fpgamif_get_bdinfo(void *hd, struct bd_cfg_info *bdinfo);
/* get fpga board running info */
int fpgamif_get_bdstat(void *hd, struct board_run_info *rinfo);
/* get fpga scan result info */
int fpgamif_get_scaninfo(void *hd, struct scan_chan_ret *sret);
/* set fpga board start, this is a global config, 
 * if fpga's DDR check ok, set board start; start: 1: start 0: stop */
int fpgamif_set_board_start(void *hd, int start);
/* set fpga board base info; dstmac, srcmac, ethtype, slot info */
int fpgamif_set_bdinfo(void *hd, struct bd_cfg_info bdinfo);
/* set fpga scan reset opt, cic: pcmid: bit15-bit5; timeslot: bit4-bit0(this opt not used timeslot) */
int fpgamif_scan_reset(void *hd, unsigned short cic);
/* set fpga scan reset opt, cic: pcmid: bit15-bit5; timeslot: bit4-bit0 */
int fpgamif_scan_start(void *hd, unsigned short cic);
int fpgamif_scan_stop(void *hd, unsigned short cic);
/* get fpga scan result, cic: pcmid: bit15-bit5; timeslot: bit4-bit0(this can't get timeslot), 
 * status: scan result 
 *   bit31-24: bit24: 0:scanning 1: scan complete; bit25: 0: scan failed 1: scan of
 *   bit23-16: scan link count, max = 2
 *   bit15-8:  the first link scan result
 *   bit7-0:   the second link scan result
 */
int fpgamif_scan_status(void *hd, unsigned short cic, unsigned int *status);
/* this is not implement */
int fpgamif_probe_link_enable(void *hd, unsigned char link);
int fpgamif_probe_link_disable(void *hd, unsigned char link);
/* config 64K channel transfer start or stop, link: link num (8bit); timeslot: 5bit */
int fpgamif_probe_channel_start(void *hd, unsigned char link, unsigned char timeslot);
int fpgamif_probe_channel_stop(void *hd, unsigned char link, unsigned char timeslot);
/* config 2M channel valid or invalid; link: 8bit; valid: 0: invalid 1: valid */
int fpgamif_write_2m_channel(void *hd, unsigned char link, unsigned char valid);
int fpgamif_read_2m_channel(void *hd, unsigned char link, unsigned char *valid);
/* config eipb phy is valid; mask: bit15~bit0: phy16~phy1 */
int fpgamif_write_e1phy_stat_port(void *hd, unsigned short mask);
int fpgamif_read_e1phy_stat_port(void *hd, unsigned short *mask);
void fpgamif_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif

