/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oiclib.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include "os.h"
#include "oic.h"
#include "oiclib.h"
#include "fst.h"
#include "errno.h"

struct oiclib_hd {
	/*dev fd */
	int fd;
	FILE *std_out;
};

FILE *oiclib_get_hd_std_out(void *hd)
{
	if (!hd) {
		return stdout;
	}

	return ((struct oiclib_hd *)hd)->std_out;
}

static int oiclib_do_conv_mac(char *str, int len, unsigned char *mac)
{
	unsigned char tmp[6];
	int i, j;
	
	if (!str || (len != 17))
		return -1;

	for (i = 0, j = 0; i < len; i += 3, j++) {
		if (i < 15) {
			if (!isxdigit(str[i]) || !isxdigit(str[i + 1]) || (str[i + 2] != ':'))
				return -1;

			tmp[j] = (char)strtol(str + i, (char **)NULL, 16);
		}
		else {
			if (!isxdigit(str[i]) || !isxdigit(str[i + 1]))
				return -1;

			tmp[j] = (char)strtol(str + i, (char **)NULL, 16);
		}
	}

	memcpy(mac, tmp, sizeof(tmp));
	return 0;
}

int oiclib_set_fpga_online(void *hd, int value)
{
	return -1;
}

int oiclib_get_fpga_online(void *hd, int *value)
{
	return -1;
}

struct fpga_verinfo *oiclib_get_fpga_version(void *hd)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_verinfo *ver = NULL;
	int rc = 0;

	if (!hd)
		return NULL;

	ver = (struct fpga_verinfo *)malloc(sizeof(*ver));
	if (!ver)
		return NULL;

	rc = ioctl(ehd->fd, OIC_GET_FPGA_VERSION, ver);
	if (rc != 0) {
		PRINTERR(hd, "get fpga version failed.\n");
		free(ver);
		return NULL;
	}

	return ver;
}

int oiclib_get_fpga_ddr_status(void *hd, int *value)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;

	if (!hd || !value)
		return -1;

	return ioctl(ehd->fd, OIC_GET_FPGA_DDR_STATUS, value);
}

int oiclib_set_fpga_bd_startup(void *hd, int value)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	
	if (!hd)
		return -1;

	return ioctl(ehd->fd, OIC_SET_FPGA_BD_STARTUP, &value);
}

int oiclib_get_fpga_bd_startup(void *hd, int *value)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;

	if (!hd || !value)
		return -1;

	return ioctl(ehd->fd, OIC_GET_FPGA_BD_STARTUP, value);
}

int oiclib_set_fpga_bd_cfginfo_orig(void *hd, char *dmac, char *smac, 
		char *etype, char *slot, char *subslot)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_board_cfginfo cfginfo;
	int ethertype = 0;
	int s, su;

	if (!hd)
		return -1;

	memset(&cfginfo, 0, sizeof(cfginfo));

	if (dmac) {
		if (oiclib_do_conv_mac(dmac, strlen(dmac), cfginfo.dstmac) != 0)
			return -1;

		cfginfo.fmask |= FMASK_DSTMAC;
	}
	if (smac) {
		if (oiclib_do_conv_mac(smac, strlen(smac), cfginfo.srcmac) != 0)
			return -1;

		cfginfo.fmask |= FMASK_SRCMAC;
	}
	if (etype) {
		ethertype = strtol(etype, NULL, 0);
		if (ethertype < 0 || ethertype > 0xffff)
			return -1;

		cfginfo.ethertype = ethertype;
		cfginfo.fmask |= FMASK_ETHERTYPE;
	}
	if (slot && subslot) {
		s = strtol(slot, NULL, 0);
		if (s < 1 || s > 14)
			return -1;

		su = strtol(subslot, NULL, 0);
		if (su < 0 || su > 4)
			return -1;
		cfginfo.slot = (s << 4) | su;
		cfginfo.fmask |= FMASK_SLOT;
	}

	return ioctl(ehd->fd, OIC_SET_FPGA_BD_CFGINFO, &cfginfo);
}

int oiclib_get_fpga_bd_cfginfo_orig(void *hd, char *dmac, char *smac,
		char *etype, char *slot, char *subslot)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_board_cfginfo cfginfo;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&cfginfo, 0, sizeof(cfginfo));

	if (dmac) {
		cfginfo.fmask |= FMASK_DSTMAC;
	}
	if (smac) {
		cfginfo.fmask |= FMASK_SRCMAC;
	}
	if (etype) {
		cfginfo.fmask |= FMASK_ETHERTYPE;
	}
	if (slot && subslot) {
		cfginfo.fmask |= FMASK_SLOT;
	}

	rc = ioctl(ehd->fd, OIC_GET_FPGA_BD_CFGINFO, &cfginfo);
	if (rc != 0) {
		return -1;
	}

	if (dmac) {
		sprintf(dmac, "%02x:%02x:%02x:%02x:%02x:%02x",
				cfginfo.dstmac[0], cfginfo.dstmac[1],
				cfginfo.dstmac[2], cfginfo.dstmac[3],
				cfginfo.dstmac[4], cfginfo.dstmac[5]);
	}
	if (smac) {
		sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x",
				cfginfo.srcmac[0], cfginfo.srcmac[1],
				cfginfo.srcmac[2], cfginfo.srcmac[3],
				cfginfo.srcmac[4], cfginfo.srcmac[5]);

	}
	if (etype) {
		sprintf(etype, "0x%04x", cfginfo.ethertype);
	}
	if (slot && subslot) {
		sprintf(slot, "%d", (cfginfo.slot >> 4) & 0xf);
		sprintf(subslot, "%d", cfginfo.slot & 0xf);
	}

	return 0;
}

int oiclib_set_fpga_bd_cfginfo_ex(void *hd, char *dmac, char *smac, 
		char *dip, char *sip, char *dport, char *sport, char *devid, char *cardid)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_board_cfginfo_ex cfginfo;
	int port;
	int id;

	if (!hd)
		return -1;

	memset(&cfginfo, 0, sizeof(cfginfo));

	if (dmac) {
		if (oiclib_do_conv_mac(dmac, strlen(dmac), cfginfo.dstmac) != 0)
			return -1;

		cfginfo.fmask |= FMASK_DSTMAC;
	}
	if (smac) {
		if (oiclib_do_conv_mac(smac, strlen(smac), cfginfo.srcmac) != 0)
			return -1;

		cfginfo.fmask |= FMASK_SRCMAC;
	}
	if (dip) {
		cfginfo.dstip = inet_network(dip);
		cfginfo.fmask |= FMASK_DSTIP;
	}
	if (sip) {
		cfginfo.srcip = inet_network(sip);
		cfginfo.fmask |= FMASK_SRCIP;
	}
	if (dport) {
		port = strtol(dport, NULL, 0);
		if (port < 0 || port > 0xffff)
			return -1;
		cfginfo.dstport = port;
		cfginfo.fmask |= FMASK_DSTPORT;
	}
	if (sport) {
		port = strtol(sport, NULL, 0);
		if (port < 0 || port > 0xffff)
			return -1;

		cfginfo.srcport = port;
		cfginfo.fmask |= FMASK_SRCPORT;
	}
	if (devid) {
		id = strtol(devid, NULL, 0);
		if (id < 1 || id > 65535)
			return -1;

		cfginfo.devid = id;
		cfginfo.fmask |= FMASK_DEVID;
	}
	if (cardid) {
		id = strtol(cardid, NULL, 0);
		if (id < 0 || id > 255)
			return -1;

		cfginfo.slot = id;
		cfginfo.fmask |= FMASK_SLOT;
	}

	return ioctl(ehd->fd, OIC_SET_FPGA_BD_CFGINFO_EX, &cfginfo);
}

int oiclib_get_fpga_bd_cfginfo_ex(void *hd, char *dmac, char *smac, 
		char *dip, char *sip, char *dport, char *sport, char *devid, char *cardid)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_board_cfginfo_ex cfginfo;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&cfginfo, 0, sizeof(cfginfo));

	if (dmac) {
		cfginfo.fmask |= FMASK_DSTMAC;
	}
	if (smac) {
		cfginfo.fmask |= FMASK_SRCMAC;
	}
	if (dip) {
		cfginfo.fmask |= FMASK_DSTIP;
	}
	if (sip) {
		cfginfo.fmask |= FMASK_SRCIP;
	}
	if (dport) {
		cfginfo.fmask |= FMASK_DSTPORT;
	}
	if (sport) {
		cfginfo.fmask |= FMASK_SRCPORT;
	}
	if (devid) {
		cfginfo.fmask |= FMASK_DEVID;
	}
	if (cardid) {
		cfginfo.fmask |= FMASK_SLOT;
	}

	rc = ioctl(ehd->fd, OIC_GET_FPGA_BD_CFGINFO_EX, &cfginfo);
	if (rc != 0) {
		return -1;
	}

	if (dmac) {
		sprintf(dmac, "%02x:%02x:%02x:%02x:%02x:%02x",
				cfginfo.dstmac[0], cfginfo.dstmac[1],
				cfginfo.dstmac[2], cfginfo.dstmac[3],
				cfginfo.dstmac[4], cfginfo.dstmac[5]);
	}
	if (smac) {
		sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x",
				cfginfo.srcmac[0], cfginfo.srcmac[1],
				cfginfo.srcmac[2], cfginfo.srcmac[3],
				cfginfo.srcmac[4], cfginfo.srcmac[5]);

	}
	if (dip) {
		sprintf(dip, "%d.%d.%d.%d", (cfginfo.dstip >> 24) & 0xff, 
				(cfginfo.dstip >> 16) & 0xff, (cfginfo.dstip >> 8) & 0xff, cfginfo.dstip & 0xff);
	}
	if (sip) {
		sprintf(sip, "%d.%d.%d.%d", (cfginfo.srcip >> 24) & 0xff, 
				(cfginfo.srcip >> 16) & 0xff, (cfginfo.srcip >> 8) & 0xff, cfginfo.srcip & 0xff);
	}
	if (dport) {
		sprintf(dport, "%u", ntohs(cfginfo.dstport));
	}
	if (sport) {
		sprintf(sport, "%u", ntohs(cfginfo.srcport));
	}
	if (devid) {
		sprintf(devid, "%u", ntohs(cfginfo.devid));
	}
	if (cardid) {
		sprintf(cardid, "%u", cfginfo.slot); 
	}

	return 0;
}

int oiclib_get_fpga_bd_runinfo(void *hd, struct fpga_board_runinfo *rinfo)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;

	if (!hd || !rinfo)
		return -1;

	return ioctl(ehd->fd, OIC_GET_FPGA_BD_RUNINFO, rinfo);
}

int oiclib_set_fpga_64k_ch_tran_start(void *hd, unsigned char link, unsigned char ts)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_trans_info tran;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&tran, 0, sizeof(tran));
	tran.channel = (ts & 0x1f) | (link << 5);
	tran.valid = 1;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_64K_CH_TRAN, &tran);
	if (rc != 0) {
		PRINTERR(hd, "set fpga 64k ch transfer start failed.\n");
		return -1;
	}

	return 0;
}

int oiclib_set_fpga_64k_ch_tran_stop(void *hd, unsigned char link, unsigned char ts)
{	
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_trans_info tran;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&tran, 0, sizeof(tran));
	tran.channel = (ts & 0x1f) | (link << 5);
	tran.valid = 0;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_64K_CH_TRAN, &tran);
	if (rc != 0) {
		PRINTERR(hd, "set fpga 64k ch transfer stop failed.\n");
		return -1;
	}

	return 0;
}

int oiclib_set_fpga_2m_ch_tran_start(void *hd, unsigned char link)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_trans_info tran;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&tran, 0, sizeof(tran));
	tran.channel = link;
	tran.valid = 1;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_2M_CH_TRAN, &tran);
	if (rc != 0) {
		PRINTERR(hd, "set fpga 2M ch transfer start failed.\n");
		return -1;
	}

	return 0;
}

int oiclib_set_fpga_2m_ch_tran_stop(void *hd, unsigned char link)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_trans_info tran;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&tran, 0, sizeof(tran));
	tran.channel = link;
	tran.valid = 0;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_2M_CH_TRAN, &tran);
	if (rc != 0) {
		PRINTERR(hd, "set fpga 2M ch transfer stop failed.\n");
		return -1;
	}

	return 0;
}

int oiclib_set_fpga_synctime(void *hd, unsigned int sec, unsigned int usec)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_time_info tinfo;

	if (!hd)
		return -1;

	memset(&tinfo, 0, sizeof(tinfo));
	tinfo.sec  = sec;
	tinfo.usec = usec;

	return ioctl(ehd->fd, OIC_SET_FPGA_SYNCTIME, &tinfo);
}

int oiclib_set_fpga_2m_ch_valid_start(void *hd, unsigned char link)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_trans_info tran;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&tran, 0, sizeof(tran));
	tran.channel = link;
	tran.valid = 1;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_2M_CH_VALID, &tran);
	if (rc != 0) {
		PRINTERR(hd, "set fpga 2M ch valid start failed.\n");
		return -1;
	}

	return 0;
}

int oiclib_set_fpga_2m_ch_valid_stop(void *hd, unsigned char link)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_trans_info tran;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&tran, 0, sizeof(tran));
	tran.channel = link;
	tran.valid = 0;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_2M_CH_VALID, &tran);
	if (rc != 0) {
		PRINTERR(hd, "set fpga 2M ch valid stop failed.\n");
		return -1;
	}

	return 0;
}

int oiclib_get_fpga_2m_ch_valid(void *hd, unsigned char link, int *valid)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_trans_info tran;
	int rc = 0;

	if (!hd || !valid)
		return -1;

	memset(&tran, 0, sizeof(tran));
	tran.channel = link;

	rc = ioctl(ehd->fd, OIC_GET_FPGA_2M_CH_VALID, &tran);
	if (rc != 0) {
		PRINTERR(hd, "get fpga 2M ch valid failed.\n");
		return -1;
	}

	*valid = tran.valid;

	return 0;
}
int oiclib_set_fpga_pl_cfgch_start(void *hd, unsigned int index, unsigned char ts,unsigned char ctl)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_cfg cfg;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&cfg, 0, sizeof(cfg));
	cfg.index=index;
	cfg.ts=ts;
	cfg.en=ctl;
	cfg.flag=1;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_PL_CFGCH, &cfg);
	if (rc != 0) {
		PRINTERR(hd, "set fpga pl cfgch start failed.\n");
		return -1;
	}

	return 0;
}
int oiclib_set_fpga_pl_cfgch_stop(void *hd, unsigned int index, unsigned char ts, unsigned char ctl)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_cfg cfg;
	int rc = 0;

	if (!hd)
		return -1;

	memset(&cfg, 0, sizeof(cfg));
	cfg.index=index;
	cfg.ts=ts;
	cfg.en=ctl;
	cfg.flag=0;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_PL_CFGCH, &cfg);
	if (rc != 0) {
		PRINTERR(hd, "set fpga pl cfgch stop failed.\n");
		return -1;
	}

	return 0;
}

int oiclib_set_fpga_pl_reset(void *hd)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	int rc = 0;

	if (!hd)
		return -1;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_PL_RSTCH, NULL);
	if (rc != 0) {
		PRINTERR(hd, "set fpga pl reset failed.\n");
		return -1;
	}

	return 0;
}
int oiclib_set_fpga_pl_tbl_info(void *hd, unsigned char slot, unsigned char subslot)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_pl_tbl_info tblinfo;
	int ret;

	if (!hd || !slot) {
		PRINTERR(hd, "set fpga pl table info failed.\n");
		return -ENOMEM;
	}

	tblinfo.slot = slot;
	tblinfo.subslot = subslot;
	ret = ioctl(ehd->fd, OIC_SET_FPGA_PL_SLOT, &tblinfo);
	if (ret != 0) {
		PRINTERR(hd, "set fpga pl table info failed.\n");
		return -1;
	}
	
	return 0;
}
int oiclib_set_fpga_pl_age_channel(void *hd, unsigned char flag)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_set_age set;
	int ret;
	
	if (!hd)
		return -1;
	if(flag != 0 && flag != 1){
		PRINTERR(hd, "set fpga pl age channel failed,because Parameter invalid,(0/1)\n");
		return -EFAULT;
	}

	set.flag = flag;
	
	ret = ioctl(ehd->fd, OIC_SET_FPGA_PL_AGE_CHANNEL, &set);
	if (ret != 0) {
		PRINTERR(hd, "set fpga pl mode failed.\n");
		return -1;
	}
	
	return 0;
}
int oiclib_set_fpga_pl_mode(void *hd, unsigned char mode)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	struct fpga_cfg cfg;
	int ret;
	
	if (!hd)
		return -1;
	
	memset(&cfg, 0, sizeof(cfg));
	
	
	cfg.mode = mode;

	ret = ioctl(ehd->fd, OIC_SET_FPGA_PL_MODE, &cfg);
	if (ret != 0) {
		PRINTERR(hd, "set fpga pl mode failed.\n");
		return -1;
	}
	
	return 0;
}
static void oiclib_get_fpga_pl_stat_done(void)
{
	FILE *st = NULL;
	char *fp = "/tmp/stat.result.done";

	st = fopen(fp,"wb");
	if (st != NULL)
		fclose(st);
}

int oiclib_get_fpga_pl_stat(void *hd)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	FILE *st = NULL;
	char *fp = "/tmp/stat.result";
	struct table tbl;
	
	if (!hd)
		return -1;
	
	memset(&tbl, 0, sizeof(tbl));

	st = fopen(fp, "wb");
	if (NULL == st) {
		PRINTERR(hd, "Failed to create file:%s.\n", fp);
		return -1;
	}

	if (ioctl(ehd->fd, OIC_GET_FPGA_PL_MATCH, &tbl) != 0) {
		PRINTERR(hd, "Failed to get stat.\n");
		fclose(st);
		return -1;
	}

	fwrite(&tbl, sizeof(tbl), 1, st);

	if (NULL != st) {
		fclose(st);
		st = NULL;
	}

	oiclib_get_fpga_pl_stat_done();

	return 0;
}

int oiclib_get_fpga_in_stat(void *hd, struct fpga_in_stat *stat)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	int rc = 0;

	if (!hd || !stat)
		return -1;

	rc = ioctl(ehd->fd, OIC_GET_FPGA_IN_STAT, stat);
	if (rc != 0) {
		PRINTERR(hd, "get fpga in stat failed.\n");
		return -1;
	}

	return 0;
}

int oiclib_get_fpga_is_sigch(void *hd, struct fpga_is_sigch *sig)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	int rc = 0;

	if (!hd || !sig)
		return -1;

	rc = ioctl(ehd->fd, OIC_GET_FPGA_IS_SIGCH, sig);
	if (rc != 0) {
		PRINTERR(hd, "get fpga signal channel failed.\n");
		return -1;
	}

	return 0;
}

void oiclib_set_hd_std_out(void *hd, FILE *fd)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	
	if (!hd)
		return ;
	ehd->std_out = fd;
}

int oiclib_set_fpga_silence_timeout(void *hd, unsigned char tm)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	int rc = 0;

	if (!hd)
		return -1;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_SILENCE_TOUT, &tm);
	if (rc != 0) {
		PRINTERR(hd, "set fpga silence timeout failed.\n");
		return -1;
	}

    return 0;
}

int oiclib_get_fpga_silence_timeout(void *hd, unsigned char *tm)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	int rc = 0;

	if (!hd || !tm)
		return -1;

	rc = ioctl(ehd->fd, OIC_GET_FPGA_SILENCE_TOUT, tm);
	if (rc != 0) {
		PRINTERR(hd, "get fpga silence timeout failed.\n");
		return -1;
	}

    return 0;
}

int oiclib_set_fpga_silence_range(void *hd, unsigned int range)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	int rc = 0;

	if (!hd)
		return -1;

	rc = ioctl(ehd->fd, OIC_SET_FPGA_SILENCE_RANGE, &range);
	if (rc != 0) {
		PRINTERR(hd, "set fpga silence range failed.\n");
		return -1;
	}

    return 0;
}

int oiclib_get_fpga_silence_range(void *hd, unsigned int *range)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
	int rc = 0;

	if (!hd || !range)
		return -1;

	rc = ioctl(ehd->fd, OIC_GET_FPGA_SILENCE_RANGE, range);
	if (rc != 0) {
		PRINTERR(hd, "get fpga silence range failed.\n");
		return -1;
	}

    return 0;
}

int oiclib_get_fpga_silence_result(void *hd, unsigned char link, unsigned char timeslot, unsigned char *stat)
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;
    struct silence_result sret;
	int rc = 0;

    if(!hd || !stat)
        return -1;

	memset(&sret, 0, sizeof(sret));
	sret.ch = (link << 5) | (timeslot & 0x1f);

	rc = ioctl(ehd->fd, OIC_GET_FPGA_SILENCE_RESULT, &sret);
	if (rc != 0) {
		PRINTERR(hd, "get fpga silence result failed.\n");
		return -1;
	}
	*stat = sret.stat;

    return 0;
}

void *oiclib_open()
{
	struct oiclib_hd *hd = NULL;
	char fname[32];

	hd = (struct oiclib_hd *)malloc(sizeof(struct oiclib_hd));
	if (!hd) {
		PRINTERR(hd, "malloc hd failed!\n");
		return NULL;
	}
	memset(hd, 0, sizeof(*hd));

	hd->std_out = stdout;
	memset(fname, 0, sizeof(fname));
	sprintf(fname, "/dev/%s", OIC_NAME);

	hd->fd = open(fname, O_RDWR);
	if (hd->fd < 0) {
		PRINTERR(hd, "open %s device failed!\n", fname);
		goto oiclib_free_exit;
	}
	
	return hd;

oiclib_free_exit:
	free(hd);
	return NULL;
}

int oiclib_close(void *hd) 
{
	struct oiclib_hd *ehd = (struct oiclib_hd *)hd;

	if (!hd)
		return -1;

	if (ehd->fd)
		close(ehd->fd);
	free(ehd);

	return 0;
}
