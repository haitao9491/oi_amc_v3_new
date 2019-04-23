/*
 * (C) Copyright 2013
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * nmclient.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "os.h"
#include "apfrm.h"
#include "aplog.h"
#include "pkt.h"
#include "utimer.h"
#include "coding.h"
#include "adapter.h"
#include "adapter_cs.h"
#include "bdtype.h"
#include "nmpkt.h"
#include "nm_typdef.h"
#include "nm_glb.h"
#include "nm_board_itf.h"

#define NMCLIENT_REPORT_INTERVAL	5
#define NMCLIENT_SERVER_PORT		28000
#define NMCLIENT_BUFSZ			1024

struct nmclient_ctl {
	struct nmboard_info bdi;

	void	*adap;
	void	*uthd;
	void	*timer;
	void	*nmboard;

	char	 svrip[32];
	int	 svrport;
	int	 connected;

	unsigned short	seqno;
};

static struct nmclient_ctl nmctl, *ctl;


void nmclient_show_usage(char *progname)
{
	printf("	--rack : \n");
	printf("	--shelf : \n");
	printf("	--slot : \n");
	printf("	--subslot : \n");
	printf("	--type : Board type (%s)\n", get_boardtype_list());
	printf("	--serverip : NMServer ip address.\n");
}

void nmclient_show_version(char *progname)
{
	printf("nmclient - V0.1\n");
}

int nmclient_parse_args(int argc, char **argv)
{
	int i = 0;

	memset(&nmctl, 0, sizeof(nmctl));
	ctl = &nmctl;

	while (i < argc) {
		if (strcmp(argv[i], "--rack") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			ctl->bdi.rack = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "--shelf") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			ctl->bdi.shelf = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "--slot") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			ctl->bdi.slot = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "--subslot") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			ctl->bdi.subslot = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "--type") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			ctl->bdi.bdtype = get_boardtype(argv[i]);
			if (ctl->bdi.bdtype == BOARD_TYPE_UNKNOWN) {
				fprintf(stderr, "Unknown board type: %s\n", argv[i]);
				return -1;
			}
		}
		else if (strcmp(argv[i], "--serverip") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			strcpy(ctl->svrip, argv[i]);
			ctl->svrport = NMCLIENT_SERVER_PORT;
		}
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return -1;
		}
		i++;
	}

	return 0;
}

void nmclient_sigalrm_handle()
{
}

void nmclient_sigusr1_handle()
{
}

void nmclient_sigusr2_handle()
{
}

static int nmclient_init_nmboard_info(int bdtype)
{
	if (bdtype == BOARD_TYPE_EIPB_V2) {
//		ctl->bdi.comp = NM_COMPONENT_E1PHY | NM_COMPONENT_FPGA | NM_COMPONENT_SWITCH;
		ctl->bdi.comp = NM_COMPONENT_E1PHY;
	}
	else if (bdtype == BOARD_TYPE_MPCB_V3) {
		ctl->bdi.comp = NM_COMPONENT_SWITCH;
	}
	else if (bdtype == BOARD_TYPE_MACB_V2) {
		ctl->bdi.comp = NM_COMPONENT_SWITCH;
	}
	else if (bdtype == BOARD_TYPE_OI_AMC_V3) {
		ctl->bdi.comp = NM_COMPONENT_FPGA;
	}
	else if (bdtype == BOARD_TYPE_EI_AMC_V1) {
		ctl->bdi.comp = NM_COMPONENT_E1PHY | NM_COMPONENT_FPGA;
	}else if (bdtype == BOARD_TYPE_MACB_V3) {
		ctl->bdi.comp = NM_COMPONENT_SWITCH;
	}
	else {
		LOG("%s: bdtype unknown %d", __func__, bdtype);
		return -1;
	}

	ctl->bdi.stat = NM_BOARD_ACTIVE;
	return 0;
}

static int nmclient_send(pkt_hdr *ph, int update)
{
	unsigned char *data = pkthdr_get_data(ph);
	unsigned char *dend = data + pkthdr_get_dlen(ph);
	pkt_hdr *mph;
	int len;
	int tcnt = 0, fcnt = 0;

	while (data < dend) {
		mph = (pkt_hdr *)data;
		len = pkthdr_get_plen(mph);

		if (update) {
			nmpkt_hdr *nmph = (nmpkt_hdr *)pkthdr_get_data(mph);

			if (nmpkthdr_get_seq(nmph) == 0) {
				nmpkthdr_set_seq(nmph, ctl->seqno);
				++(ctl->seqno);
			}
		}

		++tcnt;
		if (adapter_write(ctl->adap, mph, len) != len) {
			++fcnt;
		}

		data += len;
	}

	if (fcnt > 0) {
		LOGERROR("nmclient(send): %d of %d messages fail to be sent.", fcnt, tcnt);
		return -1;
	}

	return 0;
}

static void nmclient_timercb(unsigned int s, unsigned int ns, void *arg)
{
	struct nmclient_ctl *ctl = (struct nmclient_ctl *)arg;
	pkt_hdr *ph;

	if (ctl->connected == 1) {
		ph = nm_board_report(ctl->nmboard);
		if (ph) {
			LGWRDEBUG(ph, pkthdr_get_plen(ph), "Report pkt_hdr:");
			nmclient_send(ph, 1);
			free(ph);
		}
	}

	ctl->timer = utimer_add_by_offset(ctl->uthd, NMCLIENT_REPORT_INTERVAL, 0, ctl);
}

static void nmclient_conncb(char *lip, int lport, char *rip, int rport, void *arg)
{
	struct nmclient_ctl *ctl = (struct nmclient_ctl *)arg;

	if (ctl) {
		ctl->connected = 1;
	}
}

static void nmclient_disccb(char *lip, int lport, char *rip, int rport, void *arg)
{
	struct nmclient_ctl *ctl = (struct nmclient_ctl *)arg;

	if (ctl) {
		ctl->connected = 0;
	}
}

static void nmclient_exit_env()
{
	if (ctl->timer)
		utimer_delete(ctl->timer);
	if (ctl->uthd)
		utimer_close(ctl->uthd);
	if (ctl->nmboard)
		nm_board_close(ctl->nmboard);
	if (ctl->adap)
		adapter_close(ctl->adap);
}

static int nmclient_pkt_verify(void *buf, int size)
{
	nmpkt_hdr *nmph;
	int len;

	if (!buf)
		return -1;

	if (size < sizeof(nmpkt_hdr))
		return 0;

	nmph = (nmpkt_hdr *)buf;

	if (!nmpkthdr_valid_magic(nmph))
		return -1;

	len = sizeof(nmpkt_hdr) + nmpkthdr_get_dlen(nmph);

	if (size < len)
		return 0;

	return len;
}

static int nmclient_init_env()
{
	char cfgstr[256];

	memset(cfgstr, 0, sizeof(cfgstr));
	sprintf(cfgstr, "[NMClient.Adapter.ClientSocket]\n"
			"server=%s,%d,1024,10240,noheartbeat,payload\n",
			ctl->svrip, ctl->svrport);
	ctl->adap = adapter_register_cs_cfgstr_c(cfgstr, "NMClient.Adapter.ClientSocket",
			ap_is_running, nmclient_pkt_verify);
	if (ctl->adap == NULL) {
		LOGERROR("Failed to register adapter!");
		goto init_env_error_free;
	}
	adapter_cs_setconncb(ctl->adap, nmclient_conncb, ctl);
	adapter_cs_setdisccb(ctl->adap, nmclient_disccb, ctl);

	ctl->nmboard = nm_board_open();
	if (ctl->nmboard == NULL) {
		LOGERROR("Failed to open nmboard.");
		goto init_env_error_free;
	}

	ctl->uthd = utimer_open(UTIMER_INTERNAL_TIMESOURCE, nmclient_timercb);
	if (ctl->uthd == NULL) {
		LOGERROR("Failed to open utimer");
		goto init_env_error_free;
	}
	utimer_run(ctl->uthd, 0, 0);
	ctl->timer = utimer_add_by_offset(ctl->uthd, NMCLIENT_REPORT_INTERVAL, 0, ctl);

	adapter_open(ctl->adap);
	LOGDEBUG("nmclient init ok");

	return 0;

init_env_error_free:
	nmclient_exit_env();

	return -1;
}

static pkt_hdr *nmclient_process_global(nmpkt_hdr *nmph)
{
	unsigned char *data, *ptr;
	pkt_hdr *oph;
	int dlen = 0;

	data = (unsigned char *)malloc(NMCLIENT_BUFSZ);
	if (data == NULL) {
		LOGERROR("Global: failed to allocate data.");
		return NULL;
	}

	memset(data, 0, NMCLIENT_BUFSZ);
	ptr = data + sizeof(pkt_hdr) + sizeof(pkt_hdr) + sizeof(nmpkt_hdr);

	if (nmph->cmd == NM_CMD_GET_BOARD_INFO) {
		CODING_8(ptr, ctl->bdi.rack);
		CODING_8(ptr, ctl->bdi.shelf);
		CODING_8(ptr, ctl->bdi.slot);
		CODING_8(ptr, ctl->bdi.subslot);
		CODING_32(ptr, ctl->bdi.comp); /* component */
		CODING_8(ptr, ctl->bdi.bdtype);
		CODING_8(ptr, ctl->bdi.stat);
		dlen = 10;
	}
	else if (nmph->cmd == NM_CMD_GET_NTP_TIME) {
		CODING_8(ptr, ctl->bdi.rack);
		CODING_8(ptr, ctl->bdi.shelf);
		CODING_8(ptr, ctl->bdi.slot);
		CODING_8(ptr, ctl->bdi.subslot);
		CODING_32(ptr, time(NULL));
		dlen = 8;
	}
	else {
		LOGERROR("Global: invalid cmd: 0x%02x", nmph->cmd);
		free(data);
		return NULL;
	}

	nmpkthdr_set_dlen(nmph, dlen);
	memcpy(data + sizeof(pkt_hdr) + sizeof(pkt_hdr), nmph, sizeof(nmpkt_hdr));

	oph = (pkt_hdr *)(data + sizeof(pkt_hdr));
	pkthdr_set_sync(oph);
	pkthdr_set_plen(oph, ptr - data - sizeof(pkt_hdr));

	oph = (pkt_hdr *)data;
	pkthdr_set_sync(oph);
	pkthdr_set_plen(oph, ptr - data);

	return oph;
}

static int nmclient_process(pkt_hdr *ph)
{
	nmpkt_hdr *nmph;
	pkt_hdr *oph;
	int rc = 0;

	nmph = (nmpkt_hdr *)pkthdr_get_data(ph);
	if (nmph->module == NM_MODULE_GLB_INFO) {
		oph = nmclient_process_global(nmph);
	}
	else {
		oph = nm_board_query(ctl->nmboard, ph);
	}

	if (oph) {
		LGWRDEBUG(oph, pkthdr_get_plen(oph), "Query pkt_hdr:");
		rc = nmclient_send(oph, 0);
		free(oph);
	}

	return rc;
}

int nmclient_run(long instance, unsigned long data)
{
	pkt_hdr *ph = NULL;
	int ret = 0;

	ret = nmclient_init_nmboard_info(ctl->bdi.bdtype);
	if (ret < 0)
		return -1;

	if (nmclient_init_env() < 0) {
		return -1;
	}

	while (ap_is_running()) {
		utimer_run(ctl->uthd, 0, 0);

		ph = adapter_read(ctl->adap);
		if (ph) {
			LGWRDEBUG(ph, pkthdr_get_plen(ph), "Rcv pkt_hdr:");
			nmclient_process(ph);
			free(ph);
			continue;
		}

		SLEEP_MS(10);
	}

	nmclient_exit_env();

	return 0;
}

static struct ap_framework nmclient_app = {
	NULL,
	nmclient_run,
	0,
	nmclient_sigalrm_handle,
	nmclient_sigusr1_handle,
	nmclient_sigusr2_handle,
	nmclient_show_usage,
	nmclient_show_version,
	nmclient_parse_args
};

#if defined(__cplusplus)
extern "C" {
#endif

struct ap_framework *register_ap(void)
{
	return &nmclient_app;
}

#if defined(__cplusplus)
}
#endif

