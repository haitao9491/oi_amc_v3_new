/*
 * (C) Copyright 2016
 *
 * nmserver.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include "os.h"
#include "aplog.h"
#include "apfrm.h"
#include "utimer.h"
#include "cconfig.h"
#include "cssockdef.h"
#include "cthread.h"
#include "pkt.h"
#include "nmpkt.h"
#include "adapter.h"
#include "adapter_ss.h"
#include "nm_typdef.h"
#include "nm_glb.h"
#include "nm_client.h"
#include "nmserver.h"

#define NMSERVER_QUERY_INTERVAL 5

#define NMSERVER_LSEC "NMServer.Adapter.LServerSocket"
#define NMSERVER_RSEC "NMServer.Adapter.RServerSocket"

#define NMSERV_DEFAULT_CFGFILE  "nmserver.cfg"

struct nmserver_svr {
	int  port;
	void *thrd;
	void *adap;
	void *schd;   /* server opt client hd */
	void *glb;

	void *data;
};

struct nmserver_ctl {
	void 	*lss;  /* local board connection */
	void 	*rss;  /* web, cli, snmp client connection */
};

static struct nmserver_ctl nmctl, *ctl;
static char *cfgfile = NMSERV_DEFAULT_CFGFILE;

//TAG: nmserver_svr data~~~~~~~~~~~~~~~~~~~~~
static void nmserver_svr_set_data(void *ctl, void *data)
{
	struct nmserver_svr *ss = (struct nmserver_svr *)ctl;
	if (NULL != ss) {
		ss->data = data;
	}
}

static void *nmserver_svr_get_data(void *ctl)
{
	struct nmserver_svr *ss = (struct nmserver_svr *)ctl;
	if (NULL != ss) {
		return ss->data;
	}

	return NULL;
}

//TAG:apframe operations~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void nmserver_show_usage(char *progname)
{
	printf("        --cfgfile <string>: configuration file.\n");
}

void nmserver_show_version(char *progname)
{
	printf("nmserver - V0.1\n");
}

int nmserver_parse_args(int argc, char **argv)
{
	int i = 0;

	memset(&nmctl, 0, sizeof(nmctl));
	ctl = &nmctl;

	while (i < argc) {
		if (strcmp(argv[i], "--cfgfile") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			cfgfile = argv[i];
		}
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return -1;
		}
		i++;
	}

	return 0;
}

void nmserver_sigalrm_handle()
{
}

void nmserver_sigusr1_handle()
{
}

void nmserver_sigusr2_handle()
{
}

//TAG:nmserver operations~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static int nmpkt_verify(void *buf, int size)
{
	nmpkt_hdr *nmph = NULL;
	int len;

	if (!buf)
		return -1;

	if (size < sizeof(nmpkt_hdr))
		return 0;

	nmph = (nmpkt_hdr *)buf;

	if (!nmpkthdr_valid_magic(nmph)) {
		LOGERROR("%s: nmpkthdr magic is unvalid", __func__);
		return -1;
	}

	len = sizeof(nmpkt_hdr) + nmpkthdr_get_dlen(nmph);

	if (size < len) {
		LOGERROR("nmpkt size %d is lower nmpkt real len %d", size, len);
		return 0;
	}

	return len;
}

static int nmserver_send(void *adap, char *ip, int port, void *data)
{
	pkt_hdr *ph = (pkt_hdr *)data;

	if (!adap || !ip || !data)
		return -1;

	LGWRDEBUG(ph, pkthdr_get_plen(ph), "SEND:");

	adapter_write_to(adap, ph, pkthdr_get_plen(ph), ip, port);

	return 0;
}

void *nmserver_process(struct nmserver_svr *ss, char *ip, int port, pkt_hdr *ph)
{
	void *oph = NULL;

	if (!ss)
		return NULL;

	if (ss->port == NMSERVER_LPORT) {
		oph = nmclient_lprocess(ss->schd, ss->glb, ip, port, ph);
	}
	else if (ss->port == NMSERVER_SPORT) {
		struct nmserver_svr *lss = nmserver_svr_get_data(ss);
		oph = nmclient_rprocess(ss->schd, lss->glb, ip, port, ph);
	}

	return oph;
}

void *nmserver_proc_thread(void *arg)
{
	int port = 0;
	char ip[IP_BUF_LEN] = { 0 };
	pkt_hdr *ph = NULL, *oph = NULL;
	struct thread_arg *targ = (struct thread_arg *)arg;
	struct nmserver_svr *ss = (struct nmserver_svr *)targ->arg;

	while (ap_is_running() && NULL != ss) {
		ph = adapter_read_from(ss->adap, ip, &port);
		if (NULL != ph) {
			LGWRDEBUG(ph, pkthdr_get_plen(ph), "NMServer proc: ip %s, port %d", ip, port);
			oph = nmserver_process(ss, ip, port, ph);
			if (NULL != oph) {
				nmserver_send(ss->adap, ip, port, oph);
				free(oph);
			}

			free(ph);
			continue;
		}

		SLEEP_MS(10);
	}

	return NULL;
}

int nmserver_timer_process(void *arg)
{
	struct nmserver_ctl *ctl = (struct nmserver_ctl *)arg;
	struct nmserver_svr *lss = NULL;
	struct nmserver_svr *rss = NULL;

	if (!ctl) {
		LOGERROR("%s: ctl is null", __func__);
		return -1;
	}

	lss = ctl->lss;
	rss = ctl->rss;
	if (NULL != lss) {
		LOGDEBUG("enter lss timercb");
		//may show client && board status // or in signal to show status.
		nmclient_cmd_timer_query(lss->schd);
		//Need to check nmclient timestamp.
		nmclient_timeout_release(lss->schd, lss->glb);
	}

	if (NULL != rss) {
		LOGDEBUG("enter rss timercb");
		//Need to check nmclient timestamp.
		nmclient_timeout_release(rss->schd, rss->glb);
	}

	return 0;
}

static void nmserver_conncb(char *lip, int lport, char *rip, int rport, void *arg)
{
	struct nmserver_svr *ss = (struct nmserver_svr *)arg;

	if (!ss)
		return;

	nmclient_add_node(ss->schd, rip, rport);
	if (ss->port == NMSERVER_LPORT) {
		nms_glb_insert_bdinfo(ss->glb, rip, rport, NULL);
	}
}

static void nmserver_disccb(char *lip, int lport, char *rip, int rport, void *arg)
{
	struct nmserver_svr *ss = (struct nmserver_svr *)arg;

	if (!ss)
		return;

	nmclient_del_node(ss->schd, rip, rport);
	if (ss->port == NMSERVER_LPORT) {
		nms_glb_del_bdinfo(ss->glb, rip, rport);
	}
}

//TAG:initialize~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void nmserver_exit_env_ss(void *arg)
{
	struct nmserver_svr *ss = (struct nmserver_svr *)arg;

	if (NULL != ss) {
		if (ss->thrd)
			thread_close(ss->thrd);

		if (ss->adap)
			adapter_close(ss->adap);

		if (ss->schd)
			nmclient_ctrl_exit(ss->schd);

		if (ss->glb)
			nms_glb_close(ss->glb);

		ss->data = NULL;

		free(ss);
	}
}

void *nmserver_init_env_ss(char *cfgfile, char *section)
{
	int rc = 0;
	struct nmserver_svr *ss = NULL;
	struct adapter_ss_addr addr;

	if (!section || !cfgfile) {
		LOGERROR("%s: cfgfile or section is NULL", __func__);
		return NULL;
	}

	ss = malloc(sizeof(*ss));
	if (!ss) {
		LOGERROR("%s: malloc ss failed", __func__);
		return NULL;
	}

	memset(ss, 0, sizeof(*ss));
	ss->adap = adapter_register_ss_cfgfile_c(cfgfile, section,
			ap_is_running, nmpkt_verify);
	if (ss->adap == NULL) {
		LOGERROR("Failed to register adapter!");
		return NULL;
	}
	adapter_ss_setconncb(ss->adap, nmserver_conncb, ss);
	adapter_ss_setdisccb(ss->adap, nmserver_disccb, ss);
	LOGDEBUG("section:%s is register.", section);

	rc = adapter_open(ss->adap);
	if (0 != rc) {
		LOGERROR("Failed to open adapter!");
		goto init_ss_err;
	}

	/* get port */
	memset(&addr, 0, sizeof(addr));
	rc = adapter_ioctl(ss->adap, ADAPTER_SS_IOCTL_GETADDR, &addr);
	if (rc != 0) {
		LOGERROR("Failed to get server register port!");
		goto init_ss_err;
	}
	ss->port = addr.port;

	ss->thrd = thread_open(nmserver_proc_thread, ss);
	if (ss->thrd == NULL) {
		LOGERROR("Failed to create process thread.");
		goto init_ss_err;
	}

	ss->schd = nmclient_ctrl_init(ss->port);
	if (ss->schd == NULL) {
		LOGERROR("Failed to init server client hd.");
		goto init_ss_err;
	}

	if (ss->port == NMSERVER_LPORT) {
		ss->glb = nms_glb_open();
		if (!ss->glb) {
			LOGERROR("Failed to init server glb.");
			goto init_ss_err;
		}
	}

	return ss;

init_ss_err:
	nmserver_exit_env_ss(ss);

	return NULL;
}

static void nmserver_exit_env()
{
	if (NULL != ctl) {
		nmserver_exit_env_ss(ctl->lss);
		ctl->lss = NULL;

		nmserver_exit_env_ss(ctl->rss);
		ctl->rss = NULL;
	}
}

static int nmserver_init_env()
{
	ctl->lss = nmserver_init_env_ss(cfgfile, NMSERVER_LSEC);
	if (ctl->lss == NULL) {
		LOGERROR("Failed to init local server socket.");
		goto init_env_error_free;
	}

	ctl->rss = nmserver_init_env_ss(cfgfile, NMSERVER_RSEC);
	if (ctl->rss == NULL) {
		LOGERROR("Failed to init remote server socket.");
		goto init_env_error_free;
	}

	nmserver_svr_set_data(ctl->lss, ctl->rss);
	nmserver_svr_set_data(ctl->rss, ctl->lss);

	nmclient_ctrl_set_data(((struct nmserver_svr *)ctl->lss)->schd, ((struct nmserver_svr *)ctl->rss)->schd);
	nmclient_ctrl_set_data(((struct nmserver_svr *)ctl->rss)->schd, ((struct nmserver_svr *)ctl->lss)->schd);

	return 0;

init_env_error_free:
	nmserver_exit_env();

	return -1;
}

int nmserver_send_cmd()
{
	int ret = 1;
	struct nmserver_svr *lss = NULL;
	struct nmserver_svr *rss = NULL;
	struct nmclient_cmd *cmd = NULL;

	lss = ctl->lss;
	rss = ctl->rss;

	cmd = (struct nmclient_cmd *)nmclient_cmd_get(lss->schd);
	if (NULL != cmd) {
		nmserver_send(lss->adap, cmd->ip, cmd->port, cmd->ph);
		nmclient_cmd_release(cmd);
		ret = 0;
	}

	cmd = (struct nmclient_cmd *)nmclient_cmd_get(rss->schd);
	if (NULL != cmd) {
		nmserver_send(rss->adap, cmd->ip, cmd->port, cmd->ph);
		nmclient_cmd_release(cmd);
		ret = 0;
	}

	return ret;
}

int nmserver_run(long instance, unsigned long data)
{
	unsigned int cur = 0, pre = 0;
	struct timeval t;

	if (nmserver_init_env() < 0) {
		return -1;
	}

	while (ap_is_running()) {
		gettimeofday(&t, NULL);
		cur = (unsigned int)t.tv_sec;
		if ((cur - pre) >= NMSERVER_QUERY_INTERVAL) {
			nmserver_timer_process(ctl);
			pre = cur;
		}

		//later, send query command.
		nmserver_send_cmd();

		SLEEP_MS(10);
	}

	nmserver_exit_env();

	return 0;
}

//TAG:ap_framework~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static struct ap_framework nmserver_app = {
	NULL,
	nmserver_run,
	0,
	nmserver_sigalrm_handle,
	nmserver_sigusr1_handle,
	nmserver_sigusr2_handle,
	nmserver_show_usage,
	nmserver_show_version,
	nmserver_parse_args
};

#if defined(__cplusplus)
extern "c" {
#endif

struct ap_framework *register_ap(void)
{
	return &nmserver_app;
}

#if defined(__cplusplus)
}
#endif

