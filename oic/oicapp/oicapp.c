/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oicapp.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oiclib.h"

#define OICAPP_VERSION     "1.0"
enum FPGA_MODE{
	FPGA_MODE_ZERO=0,
	FPGA_MODE_ONE
};

enum OICAPP_ACTION_CMD {
	OICAPP_ACTION_CMD_NULL,
	OICAPP_ACTION_CMD_SHOW,
	OICAPP_ACTION_CMD_CFG
};

enum OICAPP_SUB_ACTION_CMD {
	OICAPP_SUB_ACTION_CMD_NULL,
	OICAPP_SUB_ACTION_CMD_VER,
	OICAPP_SUB_ACTION_CMD_START,
	OICAPP_SUB_ACTION_CMD_BDINFO,
	OICAPP_SUB_ACTION_CMD_BDINFOEX,
	OICAPP_SUB_ACTION_CMD_BDSTAT,
	OICAPP_SUB_ACTION_CMD_CH2M,
	OICAPP_SUB_ACTION_CMD_INSTAT,
	OICAPP_SUB_ACTION_CMD_PL_STAT,
	OICAPP_SUB_ACTION_CMD_PL_RESET,
	OICAPP_SUB_ACTION_CMD_PL_SCAN,
	OICAPP_SUB_ACTION_CMD_PL_MODE,
	OICAPP_SUB_ACTION_CMD_PL_AGE,
	OICAPP_SUB_ACTION_CMD_DMAC,
	OICAPP_SUB_ACTION_CMD_SMAC,
	OICAPP_SUB_ACTION_CMD_ETHTYPE,
	OICAPP_SUB_ACTION_CMD_SLOT,
	OICAPP_SUB_ACTION_CMD_DIP,
	OICAPP_SUB_ACTION_CMD_SIP,
	OICAPP_SUB_ACTION_CMD_DPORT,
	OICAPP_SUB_ACTION_CMD_SPORT,
	OICAPP_SUB_ACTION_CMD_DEVID,
	OICAPP_SUB_ACTION_CMD_CARDID,
	OICAPP_SUB_ACTION_CMD_TRAN,
	OICAPP_SUB_ACTION_CMD_SYNCTIME,
	OICAPP_SUB_ACTION_CMD_SLOTEX,
	OICAPP_SUB_ACTION_CMD_IS_SIGCH,
	OICAPP_SUB_ACTION_CMD_PL_TBL_INFO,
	OICAPP_SUB_ACTION_CMD_STIMEOUT,
	OICAPP_SUB_ACTION_CMD_SRANGE,
	OICAPP_SUB_ACTION_CMD_SRESULT
};

struct oicapp_data {
	void *oicapp_hd;
	enum OICAPP_ACTION_CMD action_cmd;
	enum OICAPP_SUB_ACTION_CMD sub_action_cmd;
	int enable;
	unsigned int uint1;
	unsigned int uint2;
	unsigned short usht1;
	char linkid;
	char ts;
	char chtype;/*1:2m 2:64k*/
	char *buf;
	char *buf1;
	unsigned char stimeout;
	unsigned int srange;
};

struct oicapp_data appctl, *gctl = NULL;

char str[8] = {};

static int oicapp_init_env(void)
{

	gctl->oicapp_hd = oiclib_open();
	if (NULL == gctl->oicapp_hd) {
		return -1;
	}
	gctl->action_cmd = OICAPP_ACTION_CMD_NULL;
	gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_NULL;

	return 0;
}

static void oicapp_exit_env(void)
{
	if (gctl) {
		if (gctl->oicapp_hd) {
			oiclib_close(gctl->oicapp_hd);
		}
	}
}

void oicapp_show_usage(char *name)
{
	printf("%s version %s\n", name, OICAPP_VERSION);
	printf("Usage: \n");
	printf("  %s show <string>.\n", name);
	printf("      ver: show fpga version info.\n");
	printf("      start: display board start enable config.\n");
	printf("      bdinfo: display board mac, slot config info.\n");
	printf("      bdinfoex: display board mac, ip config info.\n");
	printf("      bdstat: display board statistics info.\n");
	printf("      ch2m <0-255>: display 2m channel config info.\n");
	printf("      instat: display fpga in statistics info.\n");
	printf("      linkmap: generate linkmap file in /tmp/stat.result, it needs 2 minutes.\n");
	printf("      sigch: get channel, whether signal or voice.\n");
	printf("      stout: get silence timeout value.\n");
	printf("      srange: get silence range value.\n");
	printf("      sret <0-255> <0-31>: get phylink and timeslot of silence result value.\n");
	printf("  %s cfg <string>.\n", name);
	printf("      start [en|dis]: config board start enable or disable.\n");
	printf("      dmac xx:xx:xx:xx:xx:xx : config board dstination mac address.\n");
	printf("      smac xx:xx:xx:xx:xx:xx : config board source mac address.\n");
	printf("      ethtype <integer>: config board ethernet type.\n");
	printf("      slot <1-14> <0-4>: config board slot info, param: slot and subslot.\n");
	printf("      dport <integer>: config board destination port.\n");
	printf("      sport <integer>: config board source port.\n");
	printf("      dip <ip>: config board destination ip.\n");
	printf("      sip <ip>: config board source ip.\n");
	printf("      devid <integer>: config board device id.\n");
	printf("      cardid <integer>: config board card id.\n");
	printf("      slotex <slot> <subslot>: config ex_board slot subslot.\n");
	printf("      plinfo <slot> <subslot>: config slot and subslot info to linkmap table.\n");
	printf("      tran [2m|64k] [en|dis] <0-255> [<0-31>]: 2M or 64K channel transfer config enable and disable, param: linkid and timeslot.\n");
	printf("      ch2m [en|dis] <0-255>: 2M channel config enable and disable, param: linkid.\n");
	printf("      voice [reset|start|stop|mode|agech]\n");
	printf("      	--->start <index> <ts>:for config voice start.\n");
	printf("     	--->mode for config voice mode 0->blind sweep,1->develop a slot sweep.\n");
	printf("      synctime <second> <usecond>: config synctime.\n");
	printf("      stout: config silence timeout value.\n");
	printf("      srange: config silence range value.\n");
}

int oicapp_ver_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	struct fpga_verinfo *ver = NULL;

	if (!oic) {
		return -1;
	}

	ver = oiclib_get_fpga_version(oic->oicapp_hd);

	if (!ver) {
		return -1;
	}

	printf("version: sdh_deframe: 0x%02x, date: %02x%02x%02x%02x\n", ver->version,
			(ver->date >> 24) & 0xff, (ver->date >> 16) & 0xff, (ver->date >> 8) & 0xff, (ver->date & 0xff));

	free(ver);

	return 0;
}

int oicapp_start_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int val;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_get_fpga_ddr_status(oic->oicapp_hd, &val);
	if (rc < 0)
		return rc;
	if (val == 0) {
		printf("DDR init uncomplete, you can't set board start on this condition\n");
		return 0;
	}
	rc = oiclib_get_fpga_bd_startup(oic->oicapp_hd, &val);
	if (rc < 0)
		return rc;
	printf("board start: %s\n", val ? "enable" : "disable");

	return 0;
}

int oicapp_bdinfo_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	char dmac[18] = { 0 }, smac[18] = { 0 };
	char etype[7] = { 0 };
	char slot[3] = { 0 }, subslot[3] = { 0 };
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_get_fpga_bd_cfginfo_orig(oic->oicapp_hd, dmac, smac, etype, slot, subslot);
	if (rc < 0)
		return -1;

	printf("dstmac: %s\n", dmac);
	printf("srcmac: %s\n", smac);
	printf("ethtype: %s, slot: %s, sublot: %s\n", etype, slot, subslot);

	return 0;
}

int oicapp_bdinfo_ex_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	char dmac[18] = { 0 }, smac[18] = { 0 };
	char dip[16] = { 0 }, sip[16] = { 0 };
	char dport[6] = { 0 }, sport[6] = { 0 };
	char devid[6] = { 0 };
	char cardid[4] = { 0 };
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_get_fpga_bd_cfginfo_ex(oic->oicapp_hd, dmac, smac, dip, sip, dport, sport, devid, cardid);
	if (rc < 0)
		return -1;

	printf("dstmac:  %s\n", dmac);
	printf("srcmac:  %s\n", smac);
	printf("dstip:   %s\n", dip);
	printf("srcip:   %s\n", sip);
	printf("dstport: %s\n", dport);
	printf("srcport: %s\n", sport);
	printf("devid:   %s\n", devid);
	printf("cardid(slotex):  %s\n", cardid);

	return 0;
}

int oicapp_bdstat_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	struct fpga_board_runinfo info;
	int rc = 0;
	int i;

	if (!oic) {
		return -1;
	}

	rc = oiclib_get_fpga_bd_runinfo(oic->oicapp_hd, &info);
	if (rc < 0)
		return rc;

	printf("PORT  OPTIC_LOSS  STM1_OK  E1_SYNC_CNT  64KCHCNT  64KFRAMECNT  2MCHCNT  2MFRAMECNT\n");
	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
		printf("%-4d  %-10d  %-7d  %-11d  %-8d  %-11d  %-7d  %-11d\n",(i + 1), info.ports[i].los,
				info.ports[i].stm1_synced, info.ports[i].e1_synced_num, info.ports[i].ch_64k_num, 
				info.ports[i].ch_64k_frames, info.ports[i].ch_2m_num, info.ports[i].ch_2m_frames);
	}
	printf("\n");
	printf("ETHSTAT: %d bps\n", info.traffic);

	return 0;
}

int oicapp_ch2m_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int valid;
	int rc = 0;

	if (!oic) {
		return -1;
	}

	rc = oiclib_get_fpga_2m_ch_valid(oic->oicapp_hd, oic->linkid, &valid);
	if (rc < 0)
		return rc;
	
	printf("2M channel linkid config is %s.\n", valid ? "valid" : "invalid");

	return 0;
}

int oicapp_instat_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	struct fpga_in_stat stat;
	int rc = 0;
	int i, j;

	if (!oic) {
		return -1;
	}

	memset(&stat, 0, sizeof(stat));
	rc = oiclib_get_fpga_in_stat(oic->oicapp_hd, &stat);
	if (rc < 0)
		return rc;
	
	printf("PORT  B1    B2    B3    AUPTR  AUNDF0  AUNDF1  E1_CHANGE\n");
	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
		printf("%-6d%-6d%-6d%-6d%-7d%-8d%-8d%-9d\n", (i + 1), stat.b1_ecode[i], stat.b2_ecode[i], stat.b3_ecode[i],
										stat.auptr[i].auptr, stat.auptr[i].aundf0, stat.auptr[i].aundf1, stat.e1[i].e1_change);
	}
	printf("\n");

	printf("PORT  E1  V5_ERR  V5_PTR  V5_NDF0  V5_NDF1  E1_SYNC  E1_ERR  E1_SPEED_DIFF\n");
	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
		for (j = 0; j < FPGA_OI_E1NUM; j++) {
			printf("%-6d%-4d%-8d%-8d%-9d%-9d%-9d%-8d%-13d\n", (i + 1), (j + 1), stat.v5[i].v5_ecode[j],
							stat.v5[i].v5ptr[j], stat.v5[i].v5ndf0[j], stat.v5[i].v5ndf1[j],
							stat.e1[i].e1_sync[j], stat.e1[i].e1_sync_ecode[j], stat.e1[i].e1_speed_diff[j]);
		}
	}

	return 0;
}

int oicapp_pl_stat_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_get_fpga_pl_stat(oic->oicapp_hd);
	if (rc < 0);
		return rc;
	
	return 0;
}

int oicapp_is_sigch_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	struct fpga_is_sigch sig;
	int rc = 0;
	int i, j;

	if (!oic) {
		return -1;
	}

	memset(&sig, 0, sizeof(sig));
	rc = oiclib_get_fpga_is_sigch(oic->oicapp_hd, &sig);
	if (rc < 0) {
		return rc;
	}

	printf("E1    port1       port2       port3       port4\n");
	for (i = 0; i < 63; i++) {
		printf("%02d    ", i + 1);
		for (j = 0; j < 4; j++) {
			if (j == 3) {
				printf("%08x", sig.sig[j][i]);
			}
			else {
				printf("%08x    ", sig.sig[j][i]);
			}
		}
		printf("\n");
	}

	return 0;
}

int oicapp_silence_timeout_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	unsigned char tm = 0;
	int rc = 0;

	if (!oic) {
		return -1;
	}

	rc = oiclib_get_fpga_silence_timeout(oic->oicapp_hd, &tm);
	if (rc < 0) {
		return rc;
	}

	printf("Silence timeout: %u\n", tm);

	return 0;
}


int oicapp_silence_range_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	unsigned int range = 0;
	int rc = 0;

	if (!oic) {
		return -1;
	}

	rc = oiclib_get_fpga_silence_range(oic->oicapp_hd, &range);
	if (rc < 0) {
		return rc;
	}

	printf("Silence range: 0x%08x\n", range);

	return 0;
}

int oicapp_silence_result_show(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	unsigned char ret = 0;
	int rc = 0;

	if (!oic) {
		return -1;
	}

	rc = oiclib_get_fpga_silence_result(oic->oicapp_hd, oic->linkid, oic->ts, &ret);
	if (rc < 0) {
		return rc;
	}

	printf("Silence: phylink %u, ts %u, stat %u\n", oic->linkid, oic->ts, ret);

	return 0;
}

int oicapp_show(struct oicapp_data *data)
{
	int rc = 0;

	if (!data) {
		return -1;
	}

	switch (data->sub_action_cmd) {
		case OICAPP_SUB_ACTION_CMD_VER:
			if (oicapp_ver_show(data) < 0) {
				printf("[ERR]get fpga version.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_START:
			if (oicapp_start_show(data) < 0) {
				printf("[ERR]get start.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_BDINFO:
			if (oicapp_bdinfo_show(data) < 0) {
				printf("[ERR]get board info.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_BDINFOEX:
			if (oicapp_bdinfo_ex_show(data) < 0) {
				printf("[ERR]get board info extend.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_BDSTAT:
			if (oicapp_bdstat_show(data) < 0) {
				printf("[ERR]get board state.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_CH2M:
			if (oicapp_ch2m_show(data) < 0) {
				printf("[ERR]get 2m channel config info.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_INSTAT:
			if (oicapp_instat_show(data) < 0) {
				printf("[ERR]get fpga in state.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_PL_STAT:
			if (oicapp_pl_stat_show(data) < 0) {
				printf("[ERR]get fpga linkmap state.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_IS_SIGCH:
			if (oicapp_is_sigch_show(data) < 0) {
				printf("[ERR]get fpga signal channel info error.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_STIMEOUT:
			if (oicapp_silence_timeout_show(data) < 0) {
				printf("[ERR]get fpga silence timeout info error.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SRANGE:
			if (oicapp_silence_range_show(data) < 0) {
				printf("[ERR]get fpga silence range info error.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SRESULT:
			if (oicapp_silence_result_show(data) < 0) {
				printf("[ERR]get fpga silence result info error.\n");
			}
			break;
		default:
			rc = -1;
			break;
	}

	return rc;
}

int oicapp_start_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;
	int val;

	if (!oic) {
		return -1;
	}

	rc = oiclib_get_fpga_ddr_status(oic->oicapp_hd, &val);
	if (rc < 0)
		return rc;
	if (val == 0) {
		printf("DDR init uncomplete, you can't set board start on this condition\n");
		return 0;
	}
	rc = oiclib_set_fpga_bd_startup(oic->oicapp_hd, oic->enable);
	if (rc < 0)
		return rc;
	
	return 0;
}

int oicapp_dmac_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_orig(oic->oicapp_hd, oic->buf, NULL, NULL, NULL, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_smac_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_orig(oic->oicapp_hd, NULL, oic->buf, NULL, NULL, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_ethtype_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_orig(oic->oicapp_hd, NULL, NULL, oic->buf, NULL, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_slot_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_orig(oic->oicapp_hd, NULL, NULL, NULL, oic->buf, oic->buf1);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_dip_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_ex(oic->oicapp_hd, NULL, NULL, oic->buf, NULL, NULL, NULL, NULL, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_sip_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_ex(oic->oicapp_hd, NULL, NULL, NULL, oic->buf, NULL, NULL, NULL, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_dport_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_ex(oic->oicapp_hd, NULL, NULL, NULL, NULL, oic->buf, NULL, NULL, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_sport_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_ex(oic->oicapp_hd, NULL, NULL, NULL, NULL, NULL, oic->buf, NULL, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_devid_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_ex(oic->oicapp_hd, NULL, NULL, NULL, NULL, NULL, NULL, oic->buf, NULL);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_cardid_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_bd_cfginfo_ex(oic->oicapp_hd, NULL, NULL, NULL, NULL, NULL, NULL, NULL, oic->buf);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_tran_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	if (oic->chtype == 1) {
		if (oic->enable == 1) {
			rc = oiclib_set_fpga_2m_ch_tran_start(oic->oicapp_hd, oic->linkid);
		}
		else if (oic->enable == 0) {
			rc = oiclib_set_fpga_2m_ch_tran_stop(oic->oicapp_hd, oic->linkid);
		}
		else {
			rc = -1;
		}
	}
	else if (oic->chtype == 2) {
		if (oic->enable == 1) {
			rc = oiclib_set_fpga_64k_ch_tran_start(oic->oicapp_hd, oic->linkid, oic->ts);
		}
		else if (oic->enable == 0) {
			rc = oiclib_set_fpga_64k_ch_tran_stop(oic->oicapp_hd, oic->linkid, oic->ts);
		}
		else {
			rc = -1;
		}
	}
	else {
		rc = -1;
	}
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_ch2m_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	if (oic->enable == 1){
		rc = oiclib_set_fpga_2m_ch_valid_start(oic->oicapp_hd, oic->linkid);
	}
	else {
		rc = oiclib_set_fpga_2m_ch_valid_stop(oic->oicapp_hd, oic->linkid);
	}
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_pl_reset_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_pl_reset(oic->oicapp_hd);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_pl_scan_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	if (oic->enable == 1) {
		rc = oiclib_set_fpga_pl_cfgch_start(oic->oicapp_hd, oic->uint1, oic->uint2,1);//oic->usht1
	}
	else if (oic->enable == 0) {
		rc = oiclib_set_fpga_pl_cfgch_stop(oic->oicapp_hd, oic->uint1, oic->uint2, 0);//oic->usht1
	}
	else {
		rc = -1;
	}
	if (rc < 0)
		return rc;

	return 0;
}
int oicapp_cfg_pl_age_channle(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int ret;

	if (!oic) {
		return -1;
	}
	
	ret=oiclib_set_fpga_pl_age_channel(oic->oicapp_hd,oic->usht1);
	if(ret<0){
		return  -1;
	}
	
	return 0;
}
int oicapp_cfg_pl_mode(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int ret;

	if (!oic) {
		return -1;
	}
	ret=oiclib_set_fpga_pl_mode(oic->oicapp_hd,oic->usht1);
	if(ret<0){
		return  -1;
	}
	
	return 0;
}
int oicapp_synctime_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}
	
	rc = oiclib_set_fpga_synctime(oic->oicapp_hd, oic->uint1, oic->uint2);
	if (rc < 0)
		return rc;

	return 0;
}
int oicapp_cfg_pl_tbl_info(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	unsigned char slot, subslot;
	int rc = 0;


	if (!oic) {
		return -1;
	}
	
	slot = atoi(oic->buf);
	subslot = atoi(oic->buf1);
	rc = oiclib_set_fpga_pl_tbl_info(oic->oicapp_hd, slot, subslot);
	if (rc < 0)
		return rc;

	return 0;
}

int oicapp_silence_timeout_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}

	rc = oiclib_set_fpga_silence_timeout(oic->oicapp_hd, oic->stimeout);
	if (rc < 0)
		return rc;
	
	return 0;
}

int oicapp_silence_range_cfg(void *data)
{
	struct oicapp_data *oic = (struct oicapp_data *)data;
	int rc = 0;

	if (!oic) {
		return -1;
	}

	rc = oiclib_set_fpga_silence_range(oic->oicapp_hd, oic->srange);
	if (rc < 0)
		return rc;
	
	return 0;
}

int oicapp_cfg(struct oicapp_data *data)
{
	int rc = 0;

	if (!data) {
		return -1;
	}

	switch (data->sub_action_cmd) {
		case OICAPP_SUB_ACTION_CMD_START:
			if (oicapp_start_cfg(data) < 0) {
				printf("[ERR]config fpga start.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_DMAC:
			if (oicapp_dmac_cfg(data) < 0) {
				printf("[ERR]config destination mac.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SMAC:
			if (oicapp_smac_cfg(data) < 0) {
				printf("[ERR]config souce mac.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_ETHTYPE:
			if (oicapp_ethtype_cfg(data) < 0) {
				printf("[ERR]config ethtype.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SLOT:
			if (oicapp_slot_cfg(data) < 0) {
				printf("[ERR]config slot.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_DIP:
			if (oicapp_dip_cfg(data) < 0) {
				printf("[ERR]config destination ip.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SIP:
			if (oicapp_sip_cfg(data) < 0) {
				printf("[ERR]config source ip.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_DPORT:
			if (oicapp_dport_cfg(data) < 0) {
				printf("[ERR]config destination port.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SPORT:
			if (oicapp_sport_cfg(data) < 0) {
				printf("[ERR]config source port.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_DEVID:
			if (oicapp_devid_cfg(data) < 0) {
				printf("[ERR]config device id.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_CARDID:
			if (oicapp_cardid_cfg(data) < 0) {
				printf("[ERR]config card id.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SLOTEX:
			if (oicapp_cardid_cfg(data) < 0) {
				printf("[ERR]config card id.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_TRAN:
			if (oicapp_tran_cfg(data) < 0) {
				printf("[ERR]config transparent.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_CH2M:
			if (oicapp_ch2m_cfg(data) < 0) {
				printf("[ERR]config channel 2m valid or invalid.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_PL_RESET:
			if (oicapp_pl_reset_cfg(data) < 0) {
				printf("[ERR]config voice scan reset.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_PL_SCAN:
			if (oicapp_pl_scan_cfg(data) < 0) {
				printf("[ERR]config voice scan start or stop.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_PL_MODE:
			if(oicapp_cfg_pl_mode(data)<0)
			{
				printf("[ERR]config voice mode failed.---%s,%d\n",__FUNCTION__,__LINE__);
			}
			break;
		case OICAPP_SUB_ACTION_CMD_PL_AGE:
			if(oicapp_cfg_pl_age_channle(data)<0)
			{
				printf("[ERR]config voice age channle failed.---%s,%d\n",__FUNCTION__,__LINE__);
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SYNCTIME:
			if (oicapp_synctime_cfg(data) < 0) {
				printf("[ERR]config fpga synctime.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_PL_TBL_INFO:
			if (oicapp_cfg_pl_tbl_info(data) < 0) {
				printf("[ERR]config linkmap table info.");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_STIMEOUT:
			if (oicapp_silence_timeout_cfg(data) < 0) {
				printf("[ERR]config fpga timeout.\n");
			}
			break;
		case OICAPP_SUB_ACTION_CMD_SRANGE:
			if (oicapp_silence_range_cfg(data) < 0) {
				printf("[ERR]config fpga range.\n");
			}
			break;
		default:
			rc = -1;
			break;
	}

	return rc;
}

int oicapp_run(struct oicapp_data *data)
{
	int rc = 0;

	if (!data) {
		return -1;
	}

	switch (data->action_cmd) {
		case OICAPP_ACTION_CMD_SHOW:
			rc = oicapp_show(data);	
			break;
		case OICAPP_ACTION_CMD_CFG:
			rc = oicapp_cfg(data);
			break;
		default:
			rc = -1;
			break;
	}

	return rc;
}

int oicapp_parse_args(int argc, char **argv)
{
	int i = 0;
	int rc = 0;
	int val = 0;
	
	for (i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "show") == 0) && (argc > 2)) {
			gctl->action_cmd = OICAPP_ACTION_CMD_SHOW;
			if ((strcmp(argv[i + 1], "ver") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_VER;
			}
			else if ((strcmp(argv[i + 1], "start") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_START;
			}
			else if ((strcmp(argv[i + 1], "bdinfo") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_BDINFO;
			}
			else if ((strcmp(argv[i + 1], "bdinfoex") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_BDINFOEX;
			}
			else if ((strcmp(argv[i + 1], "bdstat") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_BDSTAT;
			}
			else if ((strcmp(argv[i + 1], "ch2m") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_CH2M;
				val = strtol(argv[i + 2], NULL, 0);
				if (val < 0 || val > 255) {
					rc = -1;
				}
				else {
					gctl->linkid = val;
				}
			}
			else if ((strcmp(argv[i + 1], "instat") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_INSTAT;
			}
			else if ((strcmp(argv[i + 1], "linkmap") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_PL_STAT;
			}
			else if ((strcmp(argv[i + 1], "sigch") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_IS_SIGCH;
			}
			else if ((strcmp(argv[i + 1], "stout") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_STIMEOUT;
			}
			else if ((strcmp(argv[i + 1], "srange") == 0) && (3 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SRANGE;
			}
			else if ((strcmp(argv[i + 1], "sret") == 0) && (5 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SRESULT;
				val = strtol(argv[i + 2], NULL, 0);
				if (val < 0 || val > 255) {
					rc = -1;
					break;
				}
				else {
					gctl->linkid = val;
				}
				val = strtol(argv[i + 3], NULL, 0);
				if (val < 0 || val > 32) {
					rc = -1;
				}
				else {
					gctl->ts = val;
				}
			}
			else {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_NULL;
				rc = -1;
			}
			break;
		}
		else if ((strcmp(argv[i], "cfg") == 0) && (argc > 2)) {
			gctl->action_cmd = OICAPP_ACTION_CMD_CFG;
			if ((strcmp(argv[i + 1], "start") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_START;
				if (strcmp(argv[i + 2], "en") == 0) {
					gctl->enable = 1;
				}
				else if (strcmp(argv[i + 2], "dis") == 0) {
					gctl->enable = 0;
				}
				else {
					rc = -1;
				}
			}
			else if ((strcmp(argv[i + 1], "dmac") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_DMAC;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "smac") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SMAC;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "ethtype") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_ETHTYPE;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "slot") == 0) && (5 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SLOT;
				if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
					return -1;
					gctl->buf = argv[i + 2];
					gctl->buf1 = argv[i + 3];
			}
			else if ((strcmp(argv[i + 1], "dip") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_DIP;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "sip") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SIP;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "dport") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_DPORT;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "sport") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SPORT;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "devid") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_DEVID;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "cardid") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_CARDID;
				gctl->buf = argv[i + 2];
			}
			else if ((strcmp(argv[i + 1], "slotex") == 0) && (5 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SLOTEX;
				memset(str, 0, sizeof(str));
				sprintf(str, "%ld", (((strtol(argv[i + 2], NULL, 0) << 4) | ((strtol(argv[i + 3], NULL, 0)) & 0xf)) & 0xff));
				gctl->buf = str;
			}
			else if ((strcmp(argv[i + 1], "plinfo") == 0) && (5 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_PL_TBL_INFO;
				if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
					return -1;
					gctl->buf = argv[i + 2];
					gctl->buf1 = argv[i + 3];
			}
			else if ((strcmp(argv[i + 1], "tran") == 0) && ((6 == argc) || (7 == argc))) {
				int val;
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_TRAN;
				if ((strcmp(argv[i + 2], "2m") == 0) && (6 == argc)) {
					gctl->chtype = 1;
				}
				else if ((strcmp(argv[i + 2], "64k") == 0) && (7 == argc)) {
					gctl->chtype = 2;
				}
				else {
					rc = -1;
					break;
				}
				if (strcmp(argv[i + 3], "en") == 0) {
					gctl->enable = 1;
				}
				else if (strcmp(argv[i + 3], "dis") == 0) {
					gctl->enable = 0;
				}
				else {
					rc = -1;
					break;
				}
				val = strtol(argv[i + 4], NULL, 0);
				if (val < 0 || val > 255) {
					rc = -1;
					break;
				}
				else {
					gctl->linkid = val;
				}
				if ((gctl->chtype == 2) && (7 == argc)) {
					val = strtol(argv[i + 5], NULL, 0);
					if (val < 0 || val > 31) {
						rc = -1;
						break;
					}
					else {
						gctl->ts = val;
					}
				}
			}
			else if ((strcmp(argv[i + 1], "ch2m") == 0) && (5 == argc)) {
				int val;
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_CH2M;
				if (strcmp(argv[i + 2], "en") == 0) {
					gctl->enable = 1;
				}
				else if (strcmp(argv[i + 2], "dis") == 0) {
					gctl->enable = 0;
				}
				else {
					rc = -1;
					break;
				}
				val = strtol(argv[i + 3], NULL, 0);
				if (val < 0 || val > 255) {
					rc = -1;
				}
				else {
					gctl->linkid = val;
				}
			}
			else if ((strcmp(argv[i + 1], "voice") == 0) && 
				(argc <= 7)) {
				if (strcmp(argv[i + 2], "reset") == 0) {
					gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_PL_RESET;
					break;
				}
				else if (strcmp(argv[i + 2], "start") == 0) {
					gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_PL_SCAN;
					if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
						return -1;
						gctl->uint1 = atoi(argv[i + 3]);
						gctl->uint2 = atoi(argv[i + 4]);
						if (((gctl->uint1 > 65536) || (gctl->uint1 < 0)) || ((gctl->uint2 > 32) || (gctl->uint2 < 0))) {
						fprintf(stderr, "Invalid mode : %s\n", argv[i]);
						return -1;
					}
					gctl->enable = 1;
				}
				else if (strcmp(argv[i + 2], "stop") == 0) {
					gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_PL_SCAN;
					gctl->enable = 0;
				}
				else if (strcmp(argv[i + 2], "mode") == 0){
					gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_PL_MODE;
					if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
						return -1;

						i++;
						gctl->usht1 = atoi(argv[i]);
						if (gctl->usht1 > 3) {
						fprintf(stderr, "Invalid mode : %s\n", argv[i]);
						return -1;
					}	
				}
				else if (strcmp(argv[i + 2], "agech") == 0) {
					gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_PL_AGE;
					gctl->usht1=1;
				}
				else {
					rc = -1;
					break;
				}
				if (7 == argc) {
					gctl->uint1 = strtol(argv[i + 3], NULL, 0);
					gctl->uint2 = strtol(argv[i + 4], NULL, 0);
					gctl->usht1 = strtol(argv[i + 5], NULL, 0);
				}
			}
			else if ((strcmp(argv[i + 1], "synctime") == 0) && (5 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SYNCTIME;
				gctl->uint1 = strtol(argv[i + 2], NULL, 0);
				gctl->uint2 = strtol(argv[i + 3], NULL, 0);
			}
			else if ((strcmp(argv[i + 1], "stout") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_STIMEOUT;
				gctl->stimeout = strtol(argv[i + 2], NULL, 0);
			}
			else if ((strcmp(argv[i + 1], "srange") == 0) && (4 == argc)) {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_SRANGE;
				gctl->srange = strtol(argv[i + 2], NULL, 0);
			}
			else {
				gctl->sub_action_cmd = OICAPP_SUB_ACTION_CMD_NULL;
				rc = -1;
			}
			break;
		}
	}
	return rc;
}

int main(int argc, char **argv)
{
	int rc = 0;
	gctl=&appctl;
	memset(gctl, 0, sizeof(appctl));

	rc = oicapp_init_env();
	if (rc < 0) {
		oicapp_show_usage(argv[0]);
		return rc;
	}

	rc = oicapp_parse_args(argc, argv);
	if (rc < 0) {
		oicapp_show_usage(argv[0]);
		oicapp_exit_env();
		return rc;
	}

	rc = oicapp_run(gctl);
	if (rc < 0) {
		oicapp_show_usage(argv[0]);
		oicapp_exit_env();
		return rc;
	}

	oicapp_exit_env();

	return 0;
}

