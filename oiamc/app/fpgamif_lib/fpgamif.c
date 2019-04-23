/*
 * (C) Copyright 2015
 * <www.raycores.com>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "fpgamif.h"
#include "fpgamif_ioctl.h"

struct dev_fd {
	int fd;
};

void *fpgamif_open(void)
{
	struct dev_fd *dev = NULL;
	char fname[32];

	dev = (struct dev_fd *)malloc(sizeof(struct dev_fd));
	if (!dev) {
		printf("devfd malloc failed\n");
		return NULL;
	}

	memset(fname, 0, sizeof(fname));
	sprintf(fname, "/dev/%s", ESNAPP_DEV_NAME);

	dev->fd = open(fname, O_RDWR);
	if (dev->fd < 0) {
		printf("open %s device err!\n", fname);
		free(dev);
		dev = NULL;
		return dev;
	}

	return dev;
}

int fpgamif_start_up(void *hd)
{
	int ret = 0;

	if (NULL != hd) {
		struct dev_fd *dev = (struct dev_fd *)hd;

		ret = ioctl(dev->fd, ESNAPP_START_UP);
		if (-1 == ret) {
			printf("[ERROR:%d] Failed to start-up FPGA.\n", __LINE__);
			return -1;
		}

		return 0;
	}

	return -1;
}

int fpgamif_get_version(void *hd, struct ver_info *ver) 
{
	if (!hd || !ver)
		return -1;

	struct dev_fd *dev = (struct dev_fd *)hd;

	if (ioctl(dev->fd, ESNAPP_RD_VERSION, ver) != 0) {
		printf("fpgamif_get_version: ioctl failed\n");
		return -1;
	}

	return 0;
}

/* 
 * start: 1: enable ; 0: disable 
 *  this must check, if board start flag in disable state, 
 *  other set may not valid.
 */
int fpgamif_get_board_start(void *hd, int *start) 
{
	if (!hd || !start) 
		return -1;

	int value;
	struct dev_fd *dev = (struct dev_fd *)hd;

	if (ioctl(dev->fd, ESNAPP_RD_DDR_INIT_STAT, &value) != 0) {
		printf("read ddr init state failed\n");
		return -1;
	}

	if (value == 0) {
		printf("DDR init uncomplete, you can't set board start on this condition\n");
		return -1;
	}

	if (ioctl(dev->fd, ESNAPP_RD_BD_START_OPT, &value) != 0) {
		printf("read board start state failed\n");
		return -1;
	}

	*start = value;
	return 0;
}

int fpgamif_get_bdinfo(void *hd, struct bd_cfg_info *bdinfo)
{
	if (!hd || !bdinfo)
		return -1;

	struct dev_fd *dev = (struct dev_fd *)hd;

	if (ioctl(dev->fd, ESNAPP_RD_BD_CFG_INFO, bdinfo) != 0) {
		printf("fpgamif_get_bdinfo: failed\n");
		return -1;
	}

	return 0;
}

int fpgamif_get_bdstat(void *hd, struct board_run_info *rinfo)
{
	if (!hd || !rinfo)
		return -1;

	struct dev_fd *dev = (struct dev_fd *)hd;

	if (ioctl(dev->fd, ESNAPP_RD_BD_RUN_INFO, rinfo) != 0) {
		printf("fpgamif_get_bdstat: get board run statistic info failed\n");
		return -1;
	}

	return 0;
}

int fpgamif_get_scaninfo(void *hd, struct scan_chan_ret *sret)
{
	if (!hd || !sret)
		return -1;

	struct dev_fd *dev = (struct dev_fd *)hd;

	if (ioctl(dev->fd, ESNAPP_RD_CFG_RESULT, sret) != 0) {
		printf("fpgamif_get_scaninfo: read scaninfo pcmid %d timeslot %d failed\n", (sret->chan >> 5), (sret->chan & 0x1f));
		return -1;
	}

	return 0;
}

int fpgamif_set_board_start(void *hd, int start)
{
	if (!hd)
		return -1;

	int ret;
	struct dev_fd *dev = (struct dev_fd *)hd;

	if (ioctl(dev->fd, ESNAPP_RD_DDR_INIT_STAT, &ret) != 0) {
		printf("fpgamif_set_board_start: read ddr init stat failed!\n");
		return -1;
	}

	if (ret == 0) {
		printf("ddr init uncomplete, can't set board start.\n");
		return -1;
	}

	if (ioctl(dev->fd, ESNAPP_WR_BD_START_OPT, &start) != 0) {
		printf("fpgamif_set_board_start: config board start failed\n");
		return -1;
	}
	
	return 0;
}

int fpgamif_set_bdinfo(void *hd, struct bd_cfg_info bdinfo)
{
	if (!hd)
		return -1;

	struct dev_fd *dev = (struct dev_fd *)hd;

	if (ioctl(dev->fd, ESNAPP_WR_BD_CFG_INFO, &bdinfo) != 0) {
		printf("fpgamif_set_bdinfo: config board info failed. %x\n", ESNAPP_WR_BD_CFG_INFO);
		return -1;
	}

	return 0;
}

/*
 * pcmid: bit15-bit5; timeslot: bit4-bit0
 */
int fpgamif_scan_reset(void *hd, unsigned short cic)
{
	if (!hd)
		return -1;

	int cfgval;
	struct dev_fd *dev = (struct dev_fd *)hd;

	cfgval = cic;
	if (ioctl(dev->fd, ESNAPP_WR_PL_SCAN_RST, &cfgval) != 0) {
		printf("config scan pcmid %d tslot %d reset failed\n", cic >> 5, cic & 0x1f);
		return -1;
	}
	return 0;
}

/*
 * pcmid: bit15-bit5; timeslot: bit4-bit0
 */
int fpgamif_scan_start(void *hd, unsigned short cic)
{
	if (!hd)
		return -1;

	int cfgval;
	struct dev_fd *dev = (struct dev_fd *)hd;

	cfgval = (1 << 16) | cic; 
	if (ioctl(dev->fd, ESNAPP_WR_VOICE_CFG, &cfgval) != 0) {
		printf("fpgamif_scan_start: config scan pcmid %d timeslot %d start failed\n", (cic >> 5), (cic & 0x1f));
		return -1;
	}

	return 0;
}

/*
 * pcmid: bit15-bit5; timeslot: bit4-bit0
 */
int fpgamif_scan_stop(void *hd, unsigned short cic)
{
	if (!hd)
		return -1;

	int cfgval;
	struct dev_fd *dev = (struct dev_fd *)hd;

	cfgval = (0 << 16) | cic;
	if (ioctl(dev->fd, ESNAPP_WR_VOICE_CFG, &cfgval) != 0) {
		printf("fpgamif_scan_start: config scan pcmid %d tslot %d stop failed\n", (cic >> 5), (cic & 0x1f));
		return -1;
	}

	return 0;
}

/* 
 * status: 
 *   bit31-24: bit24: 0:scanning 1: scan complete; bit25: 0: scan failed 1: scan of
 *   bit23-16: scan link count, max = 2
 *   bit15-8:  the first link scan result
 *   bit7-0:   the second link scan result
 */
int fpgamif_scan_status(void *hd, unsigned short cic, unsigned int *status)
{
	if (!hd || !status)
		return -1;

	struct scan_chan_ret sret;

	memset(&sret, 0, sizeof(sret));

	sret.chan = cic;
	if (fpgamif_get_scaninfo(hd, &sret) != 0) {
		return -1;
	}

	*status = (sret.scan_stat << 24) | (sret.scan_cnt << 16) | (sret.scan_ret1 << 8) | sret.scan_ret2;
	
	return 0;
}

int fpgamif_probe_link_enable(void *hd, unsigned char link)
{
	printf("fpgamif_probe_link_enable: this if not implement\n");
	return -1;
}

int fpgamif_probe_link_disable(void *hd, unsigned char link)
{
	printf("fpgamif_probe_link_disable: this if not implement\n");
	return -1;
}

int fpgamif_probe_channel_start(void *hd, unsigned char link, unsigned char timeslot)
{
	if (!hd)
		return -1;

	int value;
	struct dev_fd *dev = (struct dev_fd *)hd;

	value = (1 << 16) | (link << 5) | (timeslot & ESNAPP_TIMESLOT_MASK);
	if (ioctl(dev->fd, ESNAPP_WR_64K_CH_TRAN, &value) != 0) {
		printf("fpgamif_probe_channel_start: config link %u timeslot %u start failed\n", link, timeslot);
		return -1;
	}

	return 0;
}

int fpgamif_probe_channel_stop(void *hd, unsigned char link, unsigned char timeslot)
{
	if (!hd)
		return -1;

	int value;
	struct dev_fd *dev = (struct dev_fd *)hd;

	value = (0 << 16) | (link << 5) | (timeslot & ESNAPP_TIMESLOT_MASK);
	if (ioctl(dev->fd, ESNAPP_WR_64K_CH_TRAN, &value) != 0) {
		printf("fpgamif_probe_channel_stop: config link %u timeslot %u stop failed\n", link, timeslot);
		return -1;
	}

	return 0;
}

/* 0: invalid 1: valid */
int fpgamif_write_2m_channel(void *hd, unsigned char link, unsigned char valid)
{
	if (!hd)
		return -1;

	int value;
	struct dev_fd *dev = (struct dev_fd *)hd;

	value = (valid << 16) | link;
	if (ioctl(dev->fd, ESNAPP_WR_2M_CH_TRAN, &value) != 0) {
		printf("fpgamif_write_2m_channel: config link %u %s failed\n", link, valid ? "valid" : "invalid");
		return -1;
	}

	return 0;
}

int fpgamif_read_2m_channel(void *hd, unsigned char link, unsigned char *valid)
{
	if (!hd || !valid)
		return -1;

	int value;
	struct dev_fd *dev = (struct dev_fd *)hd;

	value = link; 
	if (ioctl(dev->fd, ESNAPP_RD_2M_CH_TRAN, &value) != 0) {
		printf("fpgamif_read_2m_channel: link %u failed\n", link);
		return -1;
	}

	*valid = (value >> 16) & 0xff;
	return 0;
}

/* bit15-0: 16-1 phy */
int fpgamif_write_e1phy_stat_port(void *hd, unsigned short mask)
{
	if (!hd)
		return -1;

	int value;
	struct dev_fd *dev = (struct dev_fd *)hd;

	value = mask;
	if (ioctl(dev->fd, ESNAPP_WR_E1PHY_STAT, &value) != 0) {
		printf("fpgamif_write_e1phy_stat_port: config port mask 0x%x failed\n", mask);
		return -1;
	}

	return 0;
}

int fpgamif_read_e1phy_stat_port(void *hd, unsigned short *mask)
{
	if (!hd)
		return -1;

	int value;
	struct dev_fd *dev = (struct dev_fd *)hd;

	if (ioctl(dev->fd, ESNAPP_RD_E1PHY_STAT, &value) != 0) {
		printf("fpgamif_read_e1phy_stat_port: read port mask failed\n");
		return -1;
	}

	*mask = value & 0xffff;
	return 0;
}

void fpgamif_close(void *hd)
{
	struct dev_fd *dev = (struct dev_fd *)hd;

	close(dev->fd);
	if (!dev)
		free(dev);
}

