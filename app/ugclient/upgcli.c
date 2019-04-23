/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * upgcli.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "apfrm.h"
#include "os.h"
#include "aplog.h"
#include "cconfig.h"
#include "pkt.h"
#include "coding.h"
#include "cthread.h"
#include "adapter.h"
#include "adapter_udp.h"
#include "utimer.h"
#include "upgdef.h"

#define BASE_ROOT	"/myself"
#define BASE_VERSION	BASE_ROOT"/version"
#define APP_ROOT	"/application"
#define APP_VERSION	APP_ROOT"/version"


struct upg_info {
	int            cmd;
	char           tftpserver[32];
	unsigned char  basever;
	char           basefile[32];
	char           basechks[32];
	unsigned char  appver;
	char           appfile[32];
	char           appchks[32];
};


static struct board_info bdinfo;
static void  *adap_ctrl;
static void  *adap_mc;
static void  *systhread = NULL;
static void  *utimer = NULL;
static void  *timer = NULL;

static struct upg_info upginfo;
static int    upgrading = 0;
static char   svrip[32];
static int    svrport;


static int upgcli_coding_msg(int cmd, unsigned char *data, int dlen, int result)
{
	unsigned char *ptr = data + sizeof(pkt_hdr);
	int len;

	memset(data, 0, dlen);

	switch (cmd) {
	case UG_UPGRADE_ACCEPT:
		{
			CODING_16(ptr, upginfo.basever);
			CODING_16(ptr, upginfo.appver);
		}
		break;

	case UG_REBOOT_ACCEPT:
		{

		}
		break;

	case UG_DNGRADE_ACCEPT:
		{
			CODING_16(ptr, upginfo.appver);
		}
		break;

	case UG_UPGRADE_RESULT:
		{
			CODING_16(ptr, upginfo.basever);
			CODING_16(ptr, upginfo.appver);
			CODING_8(ptr, result);
		}
		break;

	case UG_DNGRADE_RESULT:
		{
			CODING_16(ptr, upginfo.appver);
			CODING_8(ptr, result);
		}
		break;
	}

	len = ptr - data;

	pkthdr_set_sync((pkt_hdr *)data);
	pkthdr_set_type((pkt_hdr *)data, (cmd >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)data, (cmd >> 0) & 0xff);
	pkthdr_set_plen((pkt_hdr *)data, len);
	
	return len;
}

static void *upgcli_dosys(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	unsigned char data[256];
	int    dlen, cmd;
	char   cmdstr[1024];
	int    rc, result = 0;

	LOG("Thread(sys): starting ...");
	while (targ->flag) {
		if (upgrading == 0) {
			SLEEP_MS(100);
			continue;
		}

		/* Accept */
		if (upginfo.cmd == UG_UPGRADE_COMMAND)
			cmd = UG_UPGRADE_ACCEPT;
		else if (upginfo.cmd == UG_REBOOT_COMMAND)
			cmd = UG_REBOOT_ACCEPT;
		else
			cmd = UG_DNGRADE_ACCEPT;
		dlen = upgcli_coding_msg(cmd, data, sizeof(data), result);
		if (adapter_write_to(adap_ctrl, data, dlen, svrip, svrport) != dlen) {
			LOGERROR("Thread(sys): failed to write accept.");
		}

		/* Execute */
		memset(cmdstr, 0, sizeof(cmdstr));
		if (upginfo.cmd == UG_UPGRADE_COMMAND) {
			if (upginfo.basever == 0) {
				strcpy(upginfo.basefile, "dummy");
				strcpy(upginfo.basechks, "dummy");
			}
			if (upginfo.appver == 0) {
				strcpy(upginfo.appfile, "dummy");
				strcpy(upginfo.appchks, "dummy");
			}

			sprintf(cmdstr, "upg.sh upgrade %s %d.%d %s %s %d.%d %s %s",
					upginfo.tftpserver,
					(upginfo.basever >> 8) & 0xff, (upginfo.basever >> 0) & 0xff,
					upginfo.basefile, upginfo.basechks,
					(upginfo.appver >> 8) & 0xff, (upginfo.appver >> 0) & 0xff,
					upginfo.appfile, upginfo.appchks);
		}
		else if (upginfo.cmd == UG_REBOOT_COMMAND) {
			sprintf(cmdstr, "reboot");
		}
		else {
			sprintf(cmdstr, "upg.sh downgrade %d.%d",
					(upginfo.appver >> 8) & 0xff, (upginfo.appver >> 0) & 0xff);
		}
		LOG("cmdstr: %s", cmdstr);
		rc = system(cmdstr);

		/* Result */
		if (rc == -1)
			result = 255;
		else {
			if (WIFEXITED(rc))
				result = WEXITSTATUS(rc);
			else
				result = 254;
		}
		if (upginfo.cmd != UG_REBOOT_COMMAND) {
			if (upginfo.cmd == UG_UPGRADE_COMMAND)
				cmd = UG_UPGRADE_RESULT;
			else
				cmd = UG_DNGRADE_RESULT;
			dlen = upgcli_coding_msg(cmd, data, sizeof(data), result);
			if (adapter_write_to(adap_ctrl, data, dlen, svrip, svrport) != dlen) {
				LOGERROR("Thread(sys): failed to write reult.");
			}
		}

		upgrading = 0;
	}
	LOG("Thread(sys): stopped.");

	return NULL;
}

static void upgcli_timer_cb(unsigned int s, unsigned int ns, void *priv)
{
	unsigned char data[256], *ptr;
	unsigned int  ctrladdr = ntohl(inet_addr(bdinfo.ctrlip));
	int len;

	memset(data, 0, sizeof(data));
	ptr = data + sizeof(pkt_hdr);

	CODING_8(ptr, bdinfo.slot);
	CODING_8(ptr, bdinfo.subslot);
	CODING_8(ptr, bdinfo.type);
	CODING_16(ptr, bdinfo.basever);
	CODING_16(ptr, bdinfo.appver);
	CODING_16(ptr, bdinfo.appver_prev);
	CODING_32(ptr, ctrladdr);
	CODING_16(ptr, bdinfo.ctrlport);
	len = ptr - data;

	pkthdr_set_sync((pkt_hdr *)data);
	pkthdr_set_type((pkt_hdr *)data, (UG_NOTIFY >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)data, (UG_NOTIFY >> 0) & 0xff);
	pkthdr_set_plen((pkt_hdr *)data, len);

	if (adapter_write_to(adap_mc, data, len, MNOTIFY_ADDR, MNOTIFY_PORT) != len) {
		LOGERROR("timer(callback): failed to write.");
	}

	timer = utimer_add_by_offset(utimer, 0, 200000000, NULL);
	if (!timer) {
		LOGERROR("timer(callback): failed to add timer.");
	}
}

void upgcli_show_usage(char *progname)
{
	printf("	--slot : \n");
	printf("	--subslot : \n");
	printf("	--type : Board type (%s)\n", get_boardtype_list());
	printf("	--ctrladdr : Control ip address.\n");
}

void upgcli_show_version(char *progname)
{
	printf("upgcli - V0.1\n");
}

int upgcli_parse_args(int argc, char **argv)
{
	int i = 0;

	memset(&bdinfo, 0, sizeof(bdinfo));

	while (i < argc) {
		if (strcmp(argv[i], "--slot") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			bdinfo.slot = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "--subslot") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			bdinfo.subslot = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "--type") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			bdinfo.type = get_boardtype(argv[i]);
			if (bdinfo.type == BOARD_TYPE_UNKNOWN) {
				fprintf(stderr, "Unknown board type: %s\n", argv[i]);
				return -1;
			}
		}
		else if (strcmp(argv[i], "--ctrladdr") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			strcpy(bdinfo.ctrlip, argv[i]);
		}
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return -1;
		}
		i++;
	}

	return 0;
}

void upgcli_sigalrm_handle()
{
}

void upgcli_sigusr1_handle()
{
}

void upgcli_sigusr2_handle()
{
}

static void upgcli_exit_env()
{
	if (timer)
		utimer_delete(timer);
	if (utimer)
		utimer_close(utimer);
	if (systhread)
		thread_close(systhread);
	if (adap_mc)
		adapter_close(adap_mc);
	if (adap_ctrl)
		adapter_close(adap_ctrl);
}

static int upgcli_load_version()
{
	FILE *fp;
	char  major, minor;

	fp = fopen(BASE_VERSION, "r");
	if (fp == NULL) {
		LOGERROR("load(version): failed to open %s", BASE_VERSION);
		return -1;
	}
	if (fscanf(fp, "%hhu.%hhu\n", &major, &minor) != 2) {
		LOGERROR("load(version): failed to read %s", BASE_VERSION);
		fclose(fp);
		return -1;
	}
	bdinfo.basever = (major << 8) | minor;
	fclose(fp);

	fp = fopen(APP_VERSION, "r");
	if (fp == NULL) {
		LOGERROR("load(version): failed to open %s", APP_VERSION);
		return -1;
	}
	if (fscanf(fp, "previous=%hhu.%hhu\n", &major, &minor) != 2) {
		LOGERROR("load(version): failed to read %s", APP_VERSION);
		fclose(fp);
		return -1;
	}
	bdinfo.appver_prev = (major << 8) | minor;
	if (fscanf(fp, "current=%hhu.%hhu\n", &major, &minor) != 2) {
		LOGERROR("load(version): failed to read %s", APP_VERSION);
		fclose(fp);
		return -1;
	}
	bdinfo.appver = (major << 8) | minor;
	fclose(fp);

	return 0;
}

static int upgcli_init_env()
{
	struct udp_mcinfo mci;

	bdinfo.ctrlport = CLI_CONTROL_PORT;
	if (upgcli_load_version() != 0) {
		goto init_env_error_free;
	}

	adap_ctrl = adapter_register_udp(CLI_CONTROL_PORT, ap_is_running);
	if (!adap_ctrl) {
		LOGERROR("init: failed to register udp adapter(control).");
		goto init_env_error_free;
	}
	adapter_open(adap_ctrl);

	adap_mc = adapter_register_udp(MNOTIFY_TX_PORT, ap_is_running);
	if (!adap_mc) {
		LOGERROR("init: failed to register udp adapter(mc).");
		goto init_env_error_free;
	}
	memset(&mci, 0, sizeof(mci));
	strcpy(mci.intf, bdinfo.ctrlip);
	if (adapter_ioctl(adap_mc, ADAPTER_UDP_IOCTL_SETMCINTF, &mci) != 0) {
		LOGERROR("init: failed to SETMCINFT.");
		goto init_env_error_free;
	}
	adapter_open(adap_mc);

	systhread = thread_open(upgcli_dosys, NULL);
	if (!systhread) {
		LOGERROR("init: failed to open sys thread.");
		goto init_env_error_free;
	}

	utimer = utimer_open(UTIMER_INTERNAL_TIMESOURCE, upgcli_timer_cb);
	if (!utimer) {
		LOGERROR("init: failed to open utimer.");
		goto init_env_error_free;
	}
	utimer_run(utimer, 0, 0);
	timer = utimer_add_by_offset(utimer, 0, 200000000, NULL);
	if (!timer) {
		LOGERROR("init: failed to add timer.");
		goto init_env_error_free;
	}

	return 0;

init_env_error_free:
	upgcli_exit_env();
	return -1;
}

static int upgcli_decode_msg(pkt_hdr *ph, struct upg_info *info)
{
	unsigned char *data = pkthdr_get_data(ph);
	int            dlen = pkthdr_get_dlen(ph);
	int cmd = (pkthdr_get_type(ph) << 8) | pkthdr_get_subtype(ph);
	int rc = 0;
	unsigned int ntmp;

	memset(info, 0, sizeof(*info));
	info->cmd = cmd;

	switch (cmd) {
	case UG_UPGRADE_COMMAND:
		{
			DECODE_32(data, ntmp, dlen);
			sprintf(info->tftpserver, "%d.%d.%d.%d",
					(ntmp >> 24) & 0xff, (ntmp >> 16) & 0xff,
					(ntmp >>  8) & 0xff, (ntmp >>  0) & 0xff);
			DECODE_16(data, info->basever, dlen);
			
			if (dlen < 32) return -1;
			memcpy(info->basefile, data, 32);
			data += 32;
			dlen -= 32;

			if (dlen < 32) return -1;
			memcpy(info->basechks, data, 32);
			data += 32;
			dlen -= 32;

			DECODE_16(data, info->appver, dlen);

			if (dlen < 32) return -1;
			memcpy(info->appfile, data, 32);
			data += 32;
			dlen -= 32;

			if (dlen < 32) return -1;
			memcpy(info->appchks, data, 32);
			data += 32;
			dlen -= 32;
		}
		break;

	case UG_REBOOT_COMMAND:
		{
		}
		break;

	case UG_DNGRADE_COMMAND:
		{
			DECODE_16(data, info->appver, dlen);
		}
		break;

	default:
		rc = -1;
		break;
	}

	return rc;
}

static int upgcli_process(pkt_hdr *ph, char *ip, int port)
{
	int cmd = (pkthdr_get_type(ph) << 8) | pkthdr_get_subtype(ph);
	struct upg_info uinfo;
	int rc = -1;

	switch (cmd) {
	case UG_UPGRADE_COMMAND:
	case UG_REBOOT_COMMAND:
	case UG_DNGRADE_COMMAND:
		{
			if (!upgrading && (upgcli_decode_msg(ph, &uinfo) == 0)) {
				memcpy(&upginfo, &uinfo, sizeof(uinfo));
				strcpy(svrip, ip);
				svrport = port;
				upgrading = 1;
				rc = 0;
			}
		}
		break;

	default:
		break;
	}

	return rc;
}

int upgcli_run(long instance, unsigned long data)
{
	pkt_hdr *ph;
	char     fromip[32];
	int      fromport;

	if (upgcli_init_env() < 0)
		return -1;

	while (ap_is_running()) {
		ph = (pkt_hdr *)adapter_read_from(adap_ctrl, fromip, &fromport);
		if (ph) {
			upgcli_process(ph, fromip, fromport);
		}
		else {
			SLEEP_MS(10);
		}

		utimer_run(utimer, 0, 0);
	}

	upgcli_exit_env();

	return 0;
}

static struct ap_framework upgcli_app = {
	NULL,
	upgcli_run,
	0,
	upgcli_sigalrm_handle,
	upgcli_sigusr1_handle,
	upgcli_sigusr2_handle,
	upgcli_show_usage,
	upgcli_show_version,
	upgcli_parse_args
};

#if defined(__cplusplus)
extern "C" {
#endif

struct ap_framework *register_ap(void)
{
	return &upgcli_app;
}

#if defined(__cplusplus)
}
#endif

