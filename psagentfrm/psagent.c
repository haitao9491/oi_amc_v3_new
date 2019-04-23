/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * psagent.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "apfrm.h"
#include "os.h"
#include "aplog.h"
#include "coding.h"
#include "list.h"
#include "adapter.h"
#include "adapter_cs.h"
#include "utimer.h"
#include "mutex.h"
#include "psp.h"
#include "psagent.h"

#define SILENCE_CH_MAX   8192
#define PSAGENTVERSION "2.0"

#define PSAGENT_OPS_SCAN_INIT                scan_init
#define PSAGENT_OPS_SCAN_START               scan_start
#define PSAGENT_OPS_SCAN_STOP                scan_stop
#define PSAGENT_OPS_PROBE_LINK_ENABLE        probe_link_enable
#define PSAGENT_OPS_PROBE_LINK_DISABLE       probe_link_disable
#define PSAGENT_OPS_PROBE_CHANNEL_START      probe_channel_start
#define PSAGENT_OPS_PROBE_CHANNEL_STOP       probe_channel_stop
#define PSAGENT_OPS_PROBE_ALLCH_START        probe_allch_start
#define PSAGENT_OPS_PROBE_ALLCH_STOP         probe_allch_stop
#define PSAGENT_OPS_GET_STAT                 get_stat
#define PSAGENT_OPS_SILENCE_TIMEOUT          set_silence_timeout
#define PSAGENT_OPS_SILENCE_RANGE            set_silence_range
#define PSAGENT_OPS_PHY_STAT                 get_phy_stat 

#define PSAGENT_PROCESS(_ops)    \
	do { \
		if (pops && pops->_ops) { \
			if ((pops->_ops)(adap, mif, msg) < 0) { \
				LOGERROR("Psagent process failed."); \
			} \
		} \
	} while (0)

static char  probetype = PSP_PROBETYPE_UNKNOWN;
static char  cardtype = 0;
static char *svrip = NULL;
static int   svrport = 7181;
static void *adap = NULL;
static void *mif = NULL;
static int slot;
static int subslot;
static void *lock = NULL;
static int   connected = 0;
static int silence = 0;
static void *utimer_silence = NULL;
static unsigned long long start_count = 0;
static unsigned long long stop_count = 0;
static unsigned long long tout_count = 0;
static unsigned int running_timer = 0;
static struct silence_link *sch[SILENCE_CH_MAX];
struct psp_ops *pops = NULL;

extern struct psp_ops *psagent_ops_register(void);

int psagent_is_gsm(void) 
{
	if ((probetype == PSP_PROBETYPE_E1) || (probetype == PSP_PROBETYPE_155M_CPOS))
		return 1;

	return 0;
}

void psagent_show_usage(char *progname)
{
	printf("    --probetype <E1|155M_CPOS|155M_ATM> : Probe type of the card.\n");
	printf("    --cardtype <AMC|ATCA|OIPC> : Card type of the card.\n");
	printf("	--svrip : Server IP.\n");
	printf("	--svrport : Server port.\n");
	printf("	--slot: ATCA.SLOT\n");
	printf("	--subslot: AMC.SLOT\n");
	printf("    --silence: silence timer enable\n");
}

void psagent_show_version(char *progname)
{
	printf("psagent - %s\n", PSAGENTVERSION);
}

int psagent_parse_args(int argc, char **argv)
{
	int i = 0;

	while (i < argc) {
		if (strcmp(argv[i], "--probetype") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			if (strcmp(argv[i], "E1") == 0)
				probetype = PSP_PROBETYPE_E1;
			else if (strcmp(argv[i], "155M_CPOS") == 0)
				probetype = PSP_PROBETYPE_155M_CPOS;
			else if (strcmp(argv[i], "155M_ATM") == 0)
				probetype = PSP_PROBETYPE_155M_ATM;
			else {
				fprintf(stderr, "Invalid probetype; %s\n", argv[i]);
				return -1;
			}
		}
		else if (strcmp(argv[i], "--cardtype") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			if (strcmp(argv[i], "AMC") == 0)
				cardtype = PSP_CARDTYPE_AMC;
			else if (strcmp(argv[i], "ATCA") == 0)
				cardtype = PSP_CARDTYPE_ATCA;
			else if (strcmp(argv[i], "OIPC") == 0)
				cardtype = PSP_CARDTYPE_OIPC;
			else {
				fprintf(stderr, "Invalid cardtype: %s\n", argv[i]);
				return -1;
			}
		}
		else if (strcmp(argv[i], "--svrip") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			svrip = argv[i];
		}
		else if (strcmp(argv[i], "--svrport") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			svrport = atoi(argv[i]);
			if ((svrport <= 0) || (svrport > 65535)) {
				fprintf(stderr, "Invalid server port: %s\n", argv[i]);
				return -1;
			}
		}
		else if (strcmp(argv[i], "--slot") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			slot = atoi(argv[i]);
			if (slot <= 0) {
				fprintf(stderr, "Invalid slot : %s\n", argv[i]);
				return -1;
			}
		}
		else if (strcmp(argv[i], "--subslot") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			subslot = atoi(argv[i]);
			if ((subslot > 4)) {
				fprintf(stderr, "Invalid subslot : %s\n", argv[i]);
				return -1;
			}
		}
		else if (strcmp(argv[i], "--silence") == 0) {
			silence = 1;
		}
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return -1;
		}
		i++;
	}

	if ((probetype == PSP_PROBETYPE_UNKNOWN) || (cardtype == 0)) {
		fprintf(stderr, "Unknown probetype or cardtype.\n");
		return -1;
	}
	if (svrip == NULL) {
		fprintf(stderr, "No available server ip.\n");
		return -1;
	}

	return 0;
}

void psagent_sigalrm_handle()
{
}

void psagent_sigusr1_handle()
{
}

void psagent_sigusr2_handle()
{
}

static void psagent_exit_env()
{
	if (adap) {
		adapter_close(adap);
		adap = NULL;
	}
	if (mif) {
		pops->close(mif);
		mif = NULL;
	}
	if (lock) {
		mutex_close(lock);
		lock = NULL;
	}
}

static void psagent_conncb(char *lip, int lport, char *rip, int rport, void *arg)
{
	unsigned char buf[64], *p;
	int len;
	char s, su;

	LOG("PSAgent: Connection established.");

	len = sizeof(pkt_hdr) + 4;
	memset(buf, 0, sizeof(buf));
	pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_REGISTER >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_REGISTER >> 0) & 0xff);

	s = slot;
	su = subslot;

	p = buf + sizeof(pkt_hdr);
	CODING_8(p, probetype);
	CODING_8(p, cardtype);
	CODING_8(p, s);
	CODING_8(p, su);

	if (adapter_write(adap, buf, len) <= 0) {
		LOGERROR("Failed to send REGISTER message.");
	}

	mutex_lock(lock);
	connected = 1;
	mutex_unlock(lock);
}

static void psagent_disccb(char *lip, int lport, char *rip, int rport, void *arg)
{
	LOG("PSAgent: Connection lost.");

	mutex_lock(lock);
	connected = 0;
	mutex_unlock(lock);
}

static int psagent_silence_start_proc(struct psp_msg *msg)
{
	unsigned short index = 0;
	struct silence_link *slink = NULL; 

	slink = (struct silence_link *)malloc(sizeof(*slink));
	if (slink == NULL) {
		LOGERROR("Silence: start malloc failed.");
		return -1;
	}

	memset(slink, 0, sizeof(*slink));
	if (msg->probe.phylink < msg->probe.phylink1) {
		slink->link = msg->probe.phylink;
		slink->ts = msg->probe.channel;
		slink->link1 = msg->probe.phylink1;
		slink->ts1 = msg->probe.channel1;
	}
	else {
		slink->link = msg->probe.phylink1;
		slink->ts = msg->probe.channel1;
		slink->link1 = msg->probe.phylink;
		slink->ts1 = msg->probe.channel;
	}

	index = (slink->link << 5) | (slink->ts);
	if (index >= SILENCE_CH_MAX) {
		LOGERROR("Silence: start input link %u ts %u over max index(%u).",
				slink->link, slink->ts, index);
		free(slink);
		slink = NULL;
		return -1;
	}

	/* this process no stop and no timeout */
	LOGDEBUG("Silence(start): sch[%u] addr %p", index, sch[index]);
	if (sch[index] != NULL) {
		LOG("Silence: start proc: link:ts[%u:%u][%u:%u] restart action.", 
				slink->link, slink->ts, slink->link1, slink->ts1);
		free(slink);
		slink = NULL;
		return 0;
	}

	sch[index] = slink;
	LOGDEBUG("Silence(start): add slink to sch[%u] addr %p", index, sch[index]);

	slink->timer = utimer_add_by_offset(utimer_silence, 1, 0, slink);
	if (slink->timer == NULL) {
		LOGERROR("silence: create timer failed.");
		free(slink);
		return -1;
	}
	start_count++;
	running_timer++;

	return 0;
}

static int psagent_silence_stop_proc(struct psp_msg *msg)
{
	unsigned short index = 0;
	struct silence_link *slink = NULL; 

	if (!msg)
		return -1;

	if (msg->probe.phylink < msg->probe.phylink1) {
		index = (msg->probe.phylink << 5) | (msg->probe.channel);
	}
	else {
		index = (msg->probe.phylink1 << 5) | (msg->probe.channel1);
	}
	if (index >= SILENCE_CH_MAX) {
		LOGERROR("Silence: stop index(%u) is over the max 8192.", index);
		return -1;
	}

	LOGDEBUG("Silence(stop): sch[%u] addr %p", index, sch[index]);
	slink = sch[index];
	if (slink == NULL) {
		LOG("silence: stop [%u:%u]:[%u:%u] already absent.", 
				msg->probe.phylink, msg->probe.channel, msg->probe.phylink1, msg->probe.channel1);
		return -1;
	}

	if (slink->timer != NULL) {
		utimer_delete(slink->timer);
		slink->timer = NULL;
	}
	free(slink);
	slink = NULL;
	sch[index] = NULL;
	stop_count++;
	running_timer--;

	return 0;
}

static void psagent_silence_timercb(unsigned int s, unsigned int ns, void *data)
{
	struct silence_link *slink = (struct silence_link *)data; 
	unsigned short index = 0;
	unsigned char buf[256], *p = NULL;
	int rc = 0;
	int len = 0;

	if ((slink == NULL) || (mif == NULL) || (adap == NULL))
		return;

	index = (slink->link << 5) | slink->ts;

	rc = pops->get_silence_result(mif, slink);
	if (rc == 1) {
		slink->timer = NULL;
		slink->timer = utimer_add_by_offset(utimer_silence, 1, 0, slink);
		if (slink->timer == NULL) {
			LOGERROR("silence(timeout): create timer failed.");
			free(slink);
			slink = NULL;
		}
		return;
	}

	if (rc < 0) {
		return;
	}

	memset(buf, 0, sizeof(buf));
	len = 20 + 6;
	pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_SILENCE_RESULT >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_SILENCE_RESULT >> 0) & 0xff);

	p = buf + sizeof(pkt_hdr);
	CODING_16(p, 2);
	CODING_8(p, slink->link);
	CODING_8(p, slink->ts);
	CODING_8(p, slink->link1);
	CODING_8(p, slink->ts1);

	if (adapter_write(adap, buf, len) <= 0) {
		LOGERROR("Failed to send silence result message.");
	}
	LGWRDEBUG(buf, len, "Silence(timeoutcb):");

	slink->timer = NULL;
	free(slink);
	slink = NULL;
	sch[index] = NULL;
	tout_count++;
	running_timer--;
}

int psagent_silence_init(void)
{
	int i;

	for (i = 0; i < SILENCE_CH_MAX; i++) {
		sch[i] = NULL;
	}

	utimer_silence = utimer_open(UTIMER_INTERNAL_TIMESOURCE, psagent_silence_timercb);
	if (utimer_silence == NULL) {
		LOGERROR("silence: failed to create utimer_silence.");
		return -1;
	}
	utimer_run(utimer_silence, 0, 0);
	LOG("psagent silence initialized.");

	return 0;
}

static int psagent_init_env()
{
	char cfgstr[256];

	pops = psagent_ops_register();
	if (pops == NULL) {
		LOGERROR("Failed to register psagent operations.");
		return -1;
	}

	memset(&pkt_stat, 0, sizeof(pkt_stat));

	lock = mutex_open(NULL);
	if (lock == NULL) {
		LOGERROR("Failed to opezn mutex.");
		goto init_env_error_free;
	}

	mif = pops->open();
	if (mif == NULL) {
		LOGERROR("Failed to open fpgamif.");
		goto init_env_error_free;
	}

	if (silence == 1) {
		psagent_silence_init();
	}

	sprintf(cfgstr, "[Adapter.PSClient]\nserver=%s,%d,16384,1280\n", svrip, svrport);
	adap = adapter_register_cs_cfgstr(cfgstr, "Adapter.PSClient", ap_is_running);
	if (adap == NULL) {
		LOGERROR("Failed to register cs adapter.");
		goto init_env_error_free;
	}
	adapter_cs_setconncb(adap, psagent_conncb, NULL);
	adapter_cs_setdisccb(adap, psagent_disccb, NULL);

	adapter_open(adap);

	return 0;

init_env_error_free:
	psagent_exit_env();
	return -1;
}

static int psagent_process(struct psp_msg *msg)
{
	int ret = 0;

	switch (msg->id) {
		case PSP_MSG_SCAN_INIT:
			{
				pkt_stat.scan_init++;
				PSAGENT_PROCESS(PSAGENT_OPS_SCAN_INIT);
			}
			break;
		case PSP_MSG_SCAN_START:
			{
				pkt_stat.scan_start++;
				PSAGENT_PROCESS(PSAGENT_OPS_SCAN_START);
			}
			break;
		case PSP_MSG_SCAN_STOP:
			{
				pkt_stat.scan_stop++;
				PSAGENT_PROCESS(PSAGENT_OPS_SCAN_STOP);
			}
			break;
   		case PSP_MSG_PROBE_LINK_ENABLE:
			{
				pkt_stat.link_en++;
				PSAGENT_PROCESS(PSAGENT_OPS_PROBE_LINK_ENABLE);
			}
			break;
		case PSP_MSG_PROBE_LINK_DISABLE:
			{
				pkt_stat.link_dis++;
				PSAGENT_PROCESS(PSAGENT_OPS_PROBE_LINK_DISABLE);
			}
			break;
		case PSP_MSG_PROBE_CHANNEL_START:
			{
				pkt_stat.ch_start++;
				PSAGENT_PROCESS(PSAGENT_OPS_PROBE_CHANNEL_START);
				if (silence == 1)
					psagent_silence_start_proc(msg);
			}
			break;
		case PSP_MSG_PROBE_CHANNEL_STOP:
			{
				pkt_stat.ch_stop++;
				PSAGENT_PROCESS(PSAGENT_OPS_PROBE_CHANNEL_STOP);
				if (silence == 1)
					psagent_silence_stop_proc(msg);
			}
			break;
		case PSP_MSG_PROBE_ALLCH_START:
			{
				pkt_stat.allch_start++;
				PSAGENT_PROCESS(PSAGENT_OPS_PROBE_ALLCH_START);
			}
			break;
		case PSP_MSG_PROBE_ALLCH_STOP:
			{
				pkt_stat.allch_stop++;
				PSAGENT_PROCESS(PSAGENT_OPS_PROBE_ALLCH_STOP);
			}
			break;
		case PSP_MSG_GET_STAT:
			{
				PSAGENT_PROCESS(PSAGENT_OPS_GET_STAT);
			}
			break;
		case PSP_MSG_SILENCE_TIMEOUT:
			{
				PSAGENT_PROCESS(PSAGENT_OPS_SILENCE_TIMEOUT);
			}
			break;
		case PSP_MSG_SILENCE_RANGE:
			{
				PSAGENT_PROCESS(PSAGENT_OPS_SILENCE_RANGE);
			}
			break;
		case PSP_MSG_PHY_STAT:
            {
				PSAGENT_PROCESS(PSAGENT_OPS_PHY_STAT);
            }
            break;
		default:
			{
				pkt_stat.miss++;
				LOGERROR("Bad psp msg id:%d.", msg->id);
			}
			break;
	}

	return ret;
}

int psagent_run(long instance, unsigned long data)
{
	pkt_hdr *ph;
	struct psp_msg msg;
	unsigned long long rpkts = 0ull;
	unsigned int freq = 0;

	if (psagent_init_env() < 0)
		return -1;

	while (ap_is_running()) {
		ph = (pkt_hdr *)adapter_read(adap);
		if (ph != NULL) {
			LGWRDEBUG(ph, pkthdr_get_plen(ph), "Packet:");
			rpkts++;

			if (pkthdr_get_type(ph) != PKT_TYPE_HEARTBEAT) {
				memset(&msg, 0, sizeof(struct psp_msg));
				if (psp_decode_msg(ph, &msg) == 0) {
					psagent_process(&msg);
				}
			}

			free(ph);
		}
		
		SLEEP_MS(1);

		freq++;
		if (freq % 120000 == 0) {
			freq = 0;
            LOG("Packets: TOL %llu,INIT %llu,SST %llu,SSP %llu,LEN %llu,LDIS %llu,CST %llu,CSP %llu,AST %llu,ASP %llu,MS %llu  SEND_PKTS: get_phy_stat_ack %llu", \
					rpkts, pkt_stat.scan_init, pkt_stat.scan_start, pkt_stat.scan_stop, \
					pkt_stat.link_en, pkt_stat.link_dis, pkt_stat.ch_start, \
					pkt_stat.ch_stop, pkt_stat.allch_start, pkt_stat.allch_stop, \
					pkt_stat.miss, pkt_stat.get_phy_stat_ack);
		}
	}

	psagent_exit_env();

	return 0;
}

static struct ap_framework psagent_app = {
	NULL,
	psagent_run,
	0,
	psagent_sigalrm_handle,
	psagent_sigusr1_handle,
	psagent_sigusr2_handle,
	psagent_show_usage,
	psagent_show_version,
	psagent_parse_args
};

#if defined(__cplusplus)
extern "C" {
#endif

struct ap_framework *register_ap(void)
{
	return &psagent_app;
}

#if defined(__cplusplus)
}
#endif

