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
#include <ctype.h>
#include "fpgamif.h"
#include "fpgamif_ioctl.h"

static void usage(char *argv)
{
	fprintf(stderr, "Usage: \n");
	fprintf(stderr, "  %s start-up. start-up FPGA before config it!\n", argv);
	fprintf(stderr, "  %s show <string>.\n", argv);
	fprintf(stderr, "      ver: show fpga version info.\n");
	fprintf(stderr, "      start: display board start enable config\n");
	fprintf(stderr, "      bdinfo: display board mac, slot config info\n");
	fprintf(stderr, "      scaninfo <0-2047>: display scan status, param: pcmid\n");
	fprintf(stderr, "      bdstat : display board statistics info \n");
	fprintf(stderr, "      ch2m <0-255>: display 2m channel config info\n");
	fprintf(stderr, "      e1phy: display which e1phy is config valid \n");
	fprintf(stderr, "  %s cfg <string>.\n", argv);
	fprintf(stderr, "      start [en|dis]: config board start enable or disable\n");
	fprintf(stderr, "      dmac xx:xx:xx:xx:xx:xx : config board dstination mac address\n");
	fprintf(stderr, "      smac xx:xx:xx:xx:xx:xx : config board source mac address\n");
	fprintf(stderr, "      ethtype <integer>: config board ethernet type\n");
	fprintf(stderr, "      slot <1-14> <0-4>: config board slot info, param: slot and subslot\n");
	fprintf(stderr, "      scan en/dis <0-2047> <0-31>: config scan enable or disable, param: pcmid and timeslot\n");
	fprintf(stderr, "      scanrst <0-2047>: config scan reset, param: pcmid\n");
	fprintf(stderr, "      tran [en|dis] <0-255> <0-31>: 64K channel transfer config enable and disable, param: linkid and timeslot\n");
	fprintf(stderr, "      ch2m [en|dis] <0-255>: 2M channel config enable and disable, param: linkid\n");
	fprintf(stderr, "      e1phy <0x0000-0xffff>: config eipb e1phy port is valid or not, bit16~bit0:phy16~phy1\n");

}

static int esnapp_do_conv_mac(char *str, int len, char *mac)
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

int esnapp_do_show_version(void *fd)
{
	struct ver_info ver;

	memset(&ver, 0, sizeof(ver));

	if (fpgamif_get_version(fd, &ver) == 0) {
		printf("version: sdh_deframe: 0x%02x, date: %02x%02x%02x%02x\n", ver.ver,
				(ver.data >> 24) & 0xff, (ver.data >> 16) & 0xff, (ver.data >> 8) & 0xff, (ver.data & 0xff));
		return 0;
	}

	return -1;
}

int esnapp_do_show_start(void *fd)
{
	int start;

	if (fpgamif_get_board_start(fd, &start) != 0) {
		return -1;
	}

	printf("board start: %s\n", start ? "enable" : "disable");
	return 0;
}

int esnapp_do_show_bdinfo(void *fd)
{
	struct bd_cfg_info bdinfo;

	if (fpgamif_get_bdinfo(fd, &bdinfo) == 0) {
		printf("dstmac: %02x:%02x:%02x:%02x:%02x:%02x\n", bdinfo.dstmac[0], bdinfo.dstmac[1], 
				bdinfo.dstmac[2], bdinfo.dstmac[3], bdinfo.dstmac[4], bdinfo.dstmac[5]);
		printf("srcmac: %02x:%02x:%02x:%02x:%02x:%02x\n", bdinfo.srcmac[0], bdinfo.srcmac[1],
				bdinfo.srcmac[2], bdinfo.srcmac[3], bdinfo.srcmac[4], bdinfo.srcmac[5]);
		printf("ethtype: 0x%04x, slot: %d, subslot: %d\n", bdinfo.ethtype, 
				(bdinfo.slot >> 4) & 0x0f, bdinfo.slot & 0x0f);

		return 0;
	}

	return -1;
}

int esnapp_do_show_bdstat(void *fd)
{
	struct board_run_info bdrinfo;
	int i;

	memset(&bdrinfo, 0, sizeof(bdrinfo));

	if (fpgamif_get_bdstat(fd, &bdrinfo) != 0) {
		return -1;
	}

	printf("PORT  OPTIC_LOSS  STM1_OK  E1_SYNC_CNT  64KCHCNT  64KFRAMECNT  2MCHCNT  2MFRAMECNT\n");
	for (i = 0; i < ESNAPP_OPTIC_PORT_CNT; i++) {
		printf("%-4d  %-10d  %-7d  %-11d  %-8d  %-11d  %-7d  %-11d\n", 
				(i + 1), bdrinfo.bpi[i].optic_loss, bdrinfo.bpi[i].stm1_ok, 
				bdrinfo.bpi[i].e1_sync_cnt, bdrinfo.bpi[i].ch_cnt_64k, 
				bdrinfo.bpi[i].frame_cnt_64k, bdrinfo.bpi[i].ch_cnt_2m,
				bdrinfo.bpi[i].frame_cnt_2m);
	}

	printf("\n");
	printf("ETHSTAT: %d bps\n", bdrinfo.ethbdw);

	return 0;
}

int esnapp_do_show_scaninfo(void *fd, int pcmid)
{
	struct scan_chan_ret scanret;

	memset(&scanret, 0, sizeof(scanret));
	scanret.chan = pcmid;
	if (fpgamif_get_scaninfo(fd, &scanret) != 0) {
		return -1;
	}
	printf("PCMID  SCAN_STAT  SCAN_RESULT  SCAN_CNT  SCAN_LINK1  SCAN_LINK2\n");
	printf("%-5d  %-9d  %-11d  %-8d  %-10d  %-10d\n", 
			pcmid, (scanret.scan_stat & 0x01), (scanret.scan_stat >> 1) & 0x01, 
			scanret.scan_cnt, scanret.scan_ret1, scanret.scan_ret2);

	return 0;
}

int esnapp_do_show_ch2m(void *fd, int linkid)
{
	unsigned char link, valid;
	int rc;

	if (!fd)
		return -1;

	link = linkid & 0xff;
	rc = fpgamif_read_2m_channel(fd, link, &valid);
	if (rc < 0) 
		return -1;

	valid &= 0x01;
	printf("2M channel linkid config is %s\n", valid ? "valid" : "invalid");

	return 0;
}

int esnapp_do_show_e1phy(void *fd)
{
	unsigned short value;
	int rc;

	if (!fd)
		return -1;

	rc = fpgamif_read_e1phy_stat_port(fd, &value);
	if (rc < 0) 
		return -1;

	printf("e1phy port config is 0x%x\n", value);

	return 0;
}
int esnapp_do_show_cmd(void *fd, int argc, char *argv[])
{
	int rc = 0;
	int pcmid, linkid;

	if (argc == 1) {
		if (!strcmp(argv[0], "ver")) {
			rc = esnapp_do_show_version(fd);
		}
		else if (!strcmp(argv[0], "start")) {
			rc = esnapp_do_show_start(fd);
		}
		else if (!strcmp(argv[0], "bdinfo")) {
			rc = esnapp_do_show_bdinfo(fd);
		}
		else if (!strcmp(argv[0], "bdstat")) {
			rc = esnapp_do_show_bdstat(fd);
		}
		else if (!strcmp(argv[0], "e1phy")) {
			rc = esnapp_do_show_e1phy(fd);
		}
		else {
			rc = 1;
		}
	}
	else if (argc == 2) {
		if (!strcmp(argv[0], "ch2m")) {
			linkid = strtol(argv[1], NULL, 0);
			if (linkid < 0 || linkid > 255) {
				printf("input linkid number %d err\n", linkid);
				return -1;
			}
			rc = esnapp_do_show_ch2m(fd, linkid);
		}
		else if (!strcmp(argv[0], "scaninfo")) {
			pcmid = strtol(argv[1], NULL, 0);
			if (pcmid < 0 || pcmid > 2047) {
				printf("input pcmid number %d err\n", pcmid);
				return -1;
			}
			rc = esnapp_do_show_scaninfo(fd, pcmid);
		}
		else {
			rc = 1;
		}
	}
	else {
		rc = 1;
	}

	return rc;
}

int esnapp_do_cfg_start(void *fd, int start)
{

	if (fpgamif_set_board_start(fd, start) != 0) {
		return -1;
	}
	
	printf("config board start: %s ok!\n", start ? "enable" : "disable");
	return 0;
}

int esnapp_do_cfg_scan(void *fd, int pcmid, int tslot, int valid)
{
	unsigned short cic;

	cic = ((pcmid & 0x7ff) << 5) | (tslot & 0x1f);
	if (valid == 1) {
		return fpgamif_scan_start(fd, cic);
	}
	else {
		return fpgamif_scan_stop(fd, cic);
	}
}

int esnapp_do_cfg_scanrst(void *fd, int pcmid)
{
	unsigned short value; 

	value = (pcmid & 0x7ff);
	if (fpgamif_scan_reset(fd, value) != 0)
		return -1;

	return 0;
}

int esnapp_do_cfg_tran(void *fd, int linkid, int tslot, int valid)
{
	unsigned char link, timeslot;

	if (!fd)
		return -1;

	link = linkid & 0xff;
	timeslot = tslot & 0x1f;
	if (valid == 1) {
		return fpgamif_probe_channel_start(fd, link, timeslot);
	}
	else {
		return fpgamif_probe_channel_stop(fd, link, timeslot);
	}
}

int esnapp_do_cfg_ch2m(void *fd, int linkid, int val)
{
	unsigned char link, valid;

	if (!fd)
		return -1;

	link = linkid & 0xff;
	valid = val & 0x01;

	return fpgamif_write_2m_channel(fd, link, valid);
}

int esnapp_do_cfg_e1phy(void *fd, int port)
{
	unsigned short value;

	if (!fd)
		return -1;

	value = port & 0xffff;
	return fpgamif_write_e1phy_stat_port(fd, value);
}

int esnapp_do_cfg_cmd(void *fd, int argc, char *argv[])
{
	int rc = 0;
	int value, slot, subslot;
	struct bd_cfg_info bdinfo;
	int pcmid, tslot, linkid, port;

	memset(&bdinfo, 0, sizeof(bdinfo));

	if (argc == 2) {
		if (!strcmp(argv[0], "start")) {
			if (!strcmp(argv[1], "en")) {
				value = 1;
			}
			else if (!strcmp(argv[1], "dis")) {
				value = 0;
			}
			else {
				return 1;
			}
			rc = esnapp_do_cfg_start(fd, value);
		}
		else if (!strcmp(argv[0], "dmac")) {
			rc = esnapp_do_conv_mac(argv[1], strlen(argv[1]), bdinfo.dstmac);
			if (rc != 0) {
				printf("convert dstmac failed\n");
				return -1;
			}
			bdinfo.mask = 0x01;
			rc = fpgamif_set_bdinfo(fd, bdinfo);
		}
		else if (!strcmp(argv[0], "smac")) {
			rc = esnapp_do_conv_mac(argv[1], strlen(argv[1]), bdinfo.srcmac);
			if (rc != 0) {
				printf("convert srcmac failed\n");
				return -1;
			}
			bdinfo.mask = 0x02;
			rc = fpgamif_set_bdinfo(fd, bdinfo);
		}
		else if (!strcmp(argv[0], "ethtype")) {
			bdinfo.ethtype = strtol(argv[1], NULL, 0);
			printf("ethtype is 0x%x\n", bdinfo.ethtype);
			bdinfo.mask = 0x04;
			rc = fpgamif_set_bdinfo(fd, bdinfo);
		}
		else if (!strcmp(argv[0], "scanrst")) {
			pcmid = strtol(argv[1], NULL, 0);
			if (pcmid < 0 || pcmid > 2047) {
				printf("input channel %d err\n", pcmid);
				return -1;
			}
			rc = esnapp_do_cfg_scanrst(fd, pcmid);
		}
		else if (!strcmp(argv[0], "e1phy")) {
			port = strtol(argv[1], NULL, 0);
			if (port < 0 || port > 0xffff) {
				printf("input port num mask 0x%x err\n", port);
				return -1;
			}
			rc = esnapp_do_cfg_e1phy(fd, port);
		}
		else {
			rc = 1;
		}
	}
	else if (argc == 3) {
		if (!strcmp(argv[0], "slot")) {
			slot = strtol(argv[1], NULL, 0);
			if (slot < 1 || slot > 14) {
				printf("input slot number %d err\n", slot);
				return -1;
			}
			bdinfo.slot = (slot << 4) & 0xf0;
			subslot = strtol(argv[2], NULL, 0);
			if (subslot < 0 || subslot > 4) {
				printf("input subslot number %d err\n", subslot);
				return -1;
			}
			bdinfo.slot |= (subslot & 0x0f);
			bdinfo.mask = 0x08;
			rc = fpgamif_set_bdinfo(fd, bdinfo);
		}
		else if (!strcmp(argv[0], "ch2m")) {
			if (!strcmp(argv[1], "en")) {
				value = 1;
			}
			else if (!strcmp(argv[1], "dis")) {
				value = 0;
			}
			else {
				return 1;
			}
			linkid = strtol(argv[2], NULL, 0);
			if (linkid < 0 || linkid > 255) {
				printf("input linkid %d err\n", linkid);
				return -1;
			}
			rc = esnapp_do_cfg_ch2m(fd, linkid, value);
		}
		else {
			rc = 1;
		}
	}
	else if (argc == 4) {
		if (!strcmp(argv[0], "scan")) {
			if (!strcmp(argv[1], "en")) {
				value = 1;
			}
			else if (!strcmp(argv[1], "dis")) {
				value = 0;
			}
			else {
				return 1;
			}
			pcmid = strtol(argv[2], NULL, 0);
			if (pcmid < 0 || pcmid > 2047) {
				printf("input pcmid %d err\n", pcmid);
				return -1;
			}
			tslot = strtol(argv[3], NULL, 0);
			if (tslot < 0 || tslot > 31) {
				printf("input timeslot %d err\n", tslot);
				return -1;
			}
			rc = esnapp_do_cfg_scan(fd, pcmid, tslot, value);
		}
		else if (!strcmp(argv[0], "tran")) {
			if (!strcmp(argv[1], "en")) {
				value = 1;
			}
			else if (!strcmp(argv[1], "dis")) {
				value = 0;
			}
			else {
				return 1;
			}
			linkid = strtol(argv[2], NULL, 0);
			if (linkid < 0 || linkid > 255) {
				printf("input linkid %d err\n", linkid);
				return -1;
			}
			tslot = strtol(argv[3], NULL, 0);
			if (tslot < 0 || tslot > 31) {
				printf("input timeslot %d err\n", tslot);
				return -1;
			}
			rc = esnapp_do_cfg_tran(fd, linkid, tslot, value);
		}
		else {
			rc = 1;
		}
	}
	else {
		rc = 1;
	}

	return rc;
}

int esnapp_do_start_up_fpga(void *fd)
{
	int ret = 0;
	if (NULL != fd) {
		ret = fpgamif_start_up(fd);
		if (1 == ret && 0 == ret) {
			//1 : has start-up
			//0 : 0k.
			return 0;
		}
	}

	return -1;
}

int main(int argc, char *argv[])
{
	void *fd;
	int rc = 0;

	if (argc <= 1) {
		usage(argv[0]);
		return -1;
	}

	fd = fpgamif_open();
	if (!fd)
		return -1;

	if (!strcmp(argv[1], "show")) {
		rc = esnapp_do_show_cmd(fd, (argc - 2), &argv[2]);
	}
	else if (!strcmp(argv[1], "cfg")) {
		rc = esnapp_do_cfg_cmd(fd, (argc - 2), &argv[2]);
	}
	else if (!strcmp(argv[1], "start-up")) {
		rc = esnapp_do_start_up_fpga(fd);
	}
	else {
		usage(argv[0]);
		fpgamif_close(fd);
		return -1;
	}

	if (rc == 1) {
		usage(argv[0]);
	}

	fpgamif_close(fd);
	return 0;
}

