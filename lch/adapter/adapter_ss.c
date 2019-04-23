/*
 *
 * adapter_ss.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "os.h"
#include "aplog.h"
#include "cthread.h"
#include "cconfig.h"
#include "list.h"
#include "pkt.h"
#include "pktqueue.h"
#include "cssockdef.h"
#include "ssock.h"
#include "mpktconn.h"
#include "misc.h"
#include "adapter.h"
#include "adapter_ss.h"

#define ADAPTER_SS_NAME "Adapter.ServerSocket"

struct adapter_ss_ctl {
	unsigned int ip;
	int   port;
	int   rbs;
	int   tbs;
	int   flag;

	void *svr;
};

static void adapter_ss_exit(struct adapter_ss_ctl *ctl)
{
	if (ctl) {
		if (ctl->svr)
			ssock_close(ctl->svr);

		free(ctl);
	}
}

static int adapter_ss_init_cfg(void *adap,
		struct adapter_ss_ctl *ctl, unsigned long cfghd, char *sname)
{
	char *p, *q, value[1024];

	if (CfgGetValue(cfghd, sname, "server", value, 1, 1) == -1) {
		LOGERROR("%s: Section %s: Loading server failed.",
				adapter_get_name(adap), sname);
		return -1;
	}
	LOGINFO("%s: Section %s: Loading server: %s",
			adapter_get_name(adap), sname, value);

	p = value;

	/* ip */
	ctl->ip = get_host_addr();
	LOGINFO("%s: IP %s", adapter_get_name(adap), ip4addr_str1(ctl->ip));

	/* port */
	if (!*p) {
		LOGERROR("%s: Section %s: Loading server: No Port.",
				adapter_get_name(adap), sname);
		return -1;
	}
	if ((q = strchr(p, ',')) != NULL)
		*q = 0;
	ctl->port = atoi(p);
	p = q + 1;
	LOGINFO("%s: Section %s: Loading server: Port %d",
			adapter_get_name(adap), sname, ctl->port);

	/* rbs */
	if (!q || !*p) {
		LOGERROR("%s: Section %s: Loading server: No rbs.",
				adapter_get_name(adap), sname);
		return -1;
	}
	if ((q = strchr(p, ',')) != NULL)
		*q = 0;
	if ((ctl->rbs = atoi(p)) < 1) {
		LOGERROR("%s: Section %s: Loading server: "
				"Invalid rbs: %s.",
				adapter_get_name(adap), sname, p);
		return -1;
	}
	ctl->rbs = ctl->rbs * 1024;
	p = q + 1;
	LOGINFO("%s: Section %s: Loading server: rbs %d",
			adapter_get_name(adap), sname, ctl->rbs);

	/* tbs */
	if (!q || !*p) {
		LOGERROR("%s: Section %s: Loading server: No tbs.",
				adapter_get_name(adap), sname);
		return -1;
	}
	if ((q = strchr(p, ',')) != NULL)
		*q = 0;
	if ((ctl->tbs = atoi(p)) < 1) {
		LOGERROR("%s: Section %s: Loading server: "
				"Invalid tbs: %s.",
				adapter_get_name(adap), sname, p);
		return -1;
	}
	ctl->tbs = ctl->tbs * 1024;
	p = q + 1;
	LOGINFO("%s: Section %s: Loading server: tbs %d",
			adapter_get_name(adap), sname, ctl->tbs);

	/* flags */
	while (q && *p) {
		if ((q = strchr(p, ',')) != NULL)
			*q = 0;

		if (strcmp(p, "noheartbeat") == 0) {
			ctl->flag |= CSSOCK_NO_HEARTBEAT;
			LOGINFO("%s: Section %s: Loading server: Flag: noheartbeat",
					adapter_get_name(adap), sname);
		}
		else if (strcmp(p, "payload") == 0) {
			ctl->flag |= CSSOCK_PAYLOAD_ONLY | CSSOCK_NO_HEARTBEAT;
			LOGINFO("%s: Section %s: Loading server: Flag: payload",
					adapter_get_name(adap), sname);
		}
		else {
			LOGERROR("%s: Section %s: Loading server: "
					"Invalid flag: %s.",
					adapter_get_name(adap), sname, p);
			return -1;
		}
		p = q + 1;
	}

	return 0;
}

static struct adapter_ss_ctl *adapter_ss_init(void *adap,
		unsigned long cfghd, char *sname, int (*running)(void),
		int (*pktsync)(void *buf, int size))
{
	struct adapter_ss_ctl *ctl;

	ctl = (struct adapter_ss_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL)
		return NULL;
	memset(ctl, 0, sizeof(struct adapter_ss_ctl));

	if (adapter_ss_init_cfg(adap, ctl, cfghd, sname) < 0) {
		goto adapter_ss_init_failed;
	}

	ctl->svr = ssock_open_f(ctl->ip, ctl->port, 1, running, pktsync);
	if (ctl->svr == NULL) {
		LOGERROR("%s: Creating ssock failed.", adapter_get_name(adap));
		goto adapter_ss_init_failed;
	}
	if (pktsync)
		ctl->flag |= CSSOCK_CUSTOMIZED_PKTSYNC;
	ssock_setflag(ctl->svr, ctl->flag);
	ssock_set_bufsize(ctl->svr, ctl->rbs, ctl->tbs);
	if (ctl->port == 0) {
		ctl->port = ssock_get_port(ctl->svr);
		LOGINFO("%s: Port updated to %d", adapter_get_name(adap), ctl->port);
	}

	return ctl;

adapter_ss_init_failed:
	adapter_ss_exit(ctl);
	return NULL;
}

static int adapter_ss_open(void *adap)
{
	return 0;
}

static void *adapter_ss_read(void *adap)
{
	struct adapter_ss_ctl *ctl;

	ctl = (struct adapter_ss_ctl *)adapter_get_data(adap);
	if (!ctl)
		return NULL;

	return ssock_read(ctl->svr);
}

static void *adapter_ss_read_from(void *adap, char *ip, int *port)
{
	struct adapter_ss_ctl *ctl;

	ctl = (struct adapter_ss_ctl *)adapter_get_data(adap);
	if (!ctl)
		return NULL;

	return ssock_read_from(ctl->svr, ip, port);
}

static int adapter_ss_write(void *adap, void *data, int len)
{
	struct adapter_ss_ctl *ctl;

	ctl = (struct adapter_ss_ctl *)adapter_get_data(adap);
	if (ctl) {
		return ssock_broadcast(ctl->svr, data, len);
	}

	return -1;
}

static int adapter_ss_write_to(void *adap, void *data, int len, char *ip, int port)
{
	struct adapter_ss_ctl *ctl;

	ctl = (struct adapter_ss_ctl *)adapter_get_data(adap);
	if (ctl) {
		return ssock_write_to(ctl->svr, data, len, ip, port);
	}

	return -1;
}

static int adapter_ss_ioctl(void *adap, int code, void *arg)
{
	struct adapter_ss_ctl *ctl;
	int rc = -1;

	ctl = (struct adapter_ss_ctl *)adapter_get_data(adap);
	if (ctl) {
		switch (code) {
			case ADAPTER_SS_IOCTL_GETADDR:
				{
					struct adapter_ss_addr *addr;

					addr = (struct adapter_ss_addr *)arg;
					if (addr) {
						addr->ip = ctl->ip;
						addr->port = ctl->port;
						rc = 0;
					}
				}
				break;

			case ADAPTER_SS_IOCTL_DISCADDR:
				{
					struct adapter_ss_addr *addr;
					char buf[24] = { 0 };

					addr = (struct adapter_ss_addr *)arg;
					if (addr) {
						if ((addr->ip > 0) && (addr->port > 0)) {
							ssock_disconnect_peer(ctl->svr,
									ip4addr_str(addr->ip, buf), addr->port);
						}
					}
				}
				break;

			default:
				LOGWARN("%s: ioctl: invalid code %d.",
						adapter_get_name(adap), code);
				break;
		}
	}

	return rc;
}

static void adapter_ss_close(void *adap)
{
	adapter_ss_exit((struct adapter_ss_ctl *)adapter_get_data(adap));
}

static void *_adapter_register_ss(unsigned long cfghd, char *section,
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	void *adap;
	struct adapter_ss_ctl *ctl;

	if (!running)
		return NULL;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;
	adapter_set_name(adap, ADAPTER_SS_NAME);

	ctl = adapter_ss_init(adap, cfghd,
			section ? section: ADAPTER_SS_NAME, running, pktsync);
	if (ctl == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_data(adap, ctl);

	adapter_set_open(adap, adapter_ss_open);
	adapter_set_read(adap, adapter_ss_read);
	adapter_set_read_from(adap, adapter_ss_read_from);
	adapter_set_write(adap, adapter_ss_write);
	adapter_set_write_to(adap, adapter_ss_write_to);
	adapter_set_ioctl(adap, adapter_ss_ioctl);
	adapter_set_close(adap, adapter_ss_close);

	return adap;
}

void *adapter_register_ss(unsigned long cfghd, char *section,
		int (*running)(void))
{
	if (cfghd == 0ul)
		return NULL;

	return _adapter_register_ss(cfghd, section, running, NULL);
}

void *adapter_register_ss_cfgfile(char *cfgfile, char *section,
		int (*running)(void))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitialize(cfgfile);
	if (cfghd != 0ul) {
		p = _adapter_register_ss(cfghd, section, running, NULL);
		CfgInvalidate(cfghd);
	}

	return p;
}

void *adapter_register_ss_cfgstr(char *cfgstr, char *section,
		int (*running)(void))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitializeString(cfgstr);
	if (cfghd != 0ul) {
		p = _adapter_register_ss(cfghd, section, running, NULL);
		CfgInvalidate(cfghd);
	}

	return p;
}

void *adapter_register_ss_c(unsigned long cfghd, char *section, 
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	if (cfghd == 0ul)
		return NULL;

	return _adapter_register_ss(cfghd, section, running, pktsync);
}

void *adapter_register_ss_cfgfile_c(char *cfgfile, char *section, 
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitialize(cfgfile);
	if (cfghd != 0ul) {
		p = _adapter_register_ss(cfghd, section, running, pktsync);
		CfgInvalidate(cfghd);
	}

	return p;
}

void *adapter_register_ss_cfgstr_c(char *cfgstr, char *section, 
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitializeString(cfgstr);
	if (cfghd != 0ul) {
		p = _adapter_register_ss(cfghd, section, running, pktsync);
		CfgInvalidate(cfghd);
	}

	return p;
}

void adapter_ss_setconncb(void *adap, CSSOCK_CONN_CB conncb, void *arg)
{
	struct adapter_ss_ctl *ctl;

	ctl = (struct adapter_ss_ctl *)adapter_get_data(adap);
	if (!ctl) 
		return;

	if (ctl->svr)
		ssock_set_conncb(ctl->svr, conncb, arg);
}

void adapter_ss_setdisccb(void *adap, CSSOCK_DISC_CB disccb, void *arg)
{
	struct adapter_ss_ctl *ctl;

	ctl = (struct adapter_ss_ctl *)adapter_get_data(adap);
	if (!ctl) 
		return;

	if (ctl->svr)
		ssock_set_disccb(ctl->svr, disccb, arg);
}

