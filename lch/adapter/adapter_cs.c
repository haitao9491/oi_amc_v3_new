/*
 *
 * adapter_cs.c - A brief description goes here.
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
#include "cconfig.h"
#include "list.h"
#include "pkt.h"
#include "pktqueue.h"
#include "cssockdef.h"
#include "mpktconn.h"
#include "misc.h"
#include "adapter.h"
#include "adapter_cs.h"

#define ADAPTER_CS_NAME "Adapter.ClientSocket"

struct adapter_cs_svrinfo {
	char *ip;
	int   port;
	int   rbs;
	int   tbs;
	int   flag;

	struct list_head lst;
};

struct adapter_cs_ctl {
	struct list_head servers;

	void *mpktconn;
};

static void adapter_cs_exit(struct adapter_cs_ctl *ctl)
{
	if (ctl) {
		struct list_head *pos, *n;
		struct adapter_cs_svrinfo *entry;

		list_for_each_safe(pos, n, &(ctl->servers)) {
			entry = list_entry(pos, struct adapter_cs_svrinfo, lst);
			list_del(pos);
			if (entry->ip)
				free(entry->ip);
			free(entry);
		}

		if (ctl->mpktconn)
			mpktconn_close(ctl->mpktconn);

		free(ctl);
	}
}

static int adapter_cs_init_cfg(void *adap,
		struct adapter_cs_ctl *ctl, unsigned long cfghd, char *sname)
{
	int   i, idx;
	char *p, *q, value[1024];

	i = CfgGetCount(cfghd, sname, "server", 1);
	if (i < 1) {
		LOGERROR2("%s: Section %s: No server defined.",
			       adapter_get_name(adap), sname);
		return -1;
	}

	for (idx = 1; idx <= i; idx++) {
		struct adapter_cs_svrinfo *svr;

		if (CfgGetValue(cfghd, sname, "server", value, idx, 1) == -1) {
			LOGERROR3("%s: Section %s: Loading server %d failed.",
					adapter_get_name(adap), sname, idx);
			return -1;
		}
		LOGINFO("%s: Section %s: Loading server %d: %s",
				adapter_get_name(adap), sname, idx, value);

		svr = (struct adapter_cs_svrinfo *)malloc(sizeof(*svr));
		if (svr == NULL) {
			LOGERROR3("%s: Section %s: Loading server %d: Insufficient memory.", adapter_get_name(adap), sname, idx);
			return -1;
		}
		memset(svr, 0, sizeof(*svr));
		list_add(&(svr->lst), &(ctl->servers));

		p = value;

		/* ip */
		if (!*p) {
			LOGERROR3("%s: Section %s: Loading server %d: No IP.",
					adapter_get_name(adap), sname, idx);
			return -1;
		}
		if ((q = strchr(p, ',')) != NULL)
			*q = 0;
		svr->ip = strdup(p);
		if (svr->ip == NULL) {
			LOGERROR3("%s: Section %s: Loading server %d: Insufficient memory.", adapter_get_name(adap), sname, idx);
			return -1;
		}
		p = q + 1;
		LOGINFO("%s: Section %s: Loading server %d: IP %s",
				adapter_get_name(adap), sname, idx, svr->ip);

		/* port */
		if (!q || !*p) {
			LOGERROR3("%s: Section %s: Loading server %d: No Port.",
					adapter_get_name(adap), sname, idx);
			return -1;
		}
		if ((q = strchr(p, ',')) != NULL)
			*q = 0;
		svr->port = atoi(p);
		p = q + 1;
		LOGINFO("%s: Section %s: Loading server %d: Port %d",
				adapter_get_name(adap), sname, idx,svr->port);

		/* rbs */
		if (!q || !*p) {
			LOGERROR3("%s: Section %s: Loading server %d: No rbs.",
					adapter_get_name(adap), sname, idx);
			return -1;
		}
		if ((q = strchr(p, ',')) != NULL)
			*q = 0;
		if ((svr->rbs = atoi(p)) < 1) {
			LOGERROR4("%s: Section %s: Loading server %d: "
					"Invalid rbs: %s.",
					adapter_get_name(adap), sname, idx, p);
			return -1;
		}
		svr->rbs = svr->rbs * 1024;
		p = q + 1;
		LOGINFO("%s: Section %s: Loading server %d: rbs %d",
				adapter_get_name(adap), sname, idx, svr->rbs);

		/* tbs */
		if (!q || !*p) {
			LOGERROR3("%s: Section %s: Loading server %d: No tbs.",
					adapter_get_name(adap), sname, idx);
			return -1;
		}
		if ((q = strchr(p, ',')) != NULL)
			*q = 0;
		if ((svr->tbs = atoi(p)) < 1) {
			LOGERROR4("%s: Section %s: Loading server %d: "
					"Invalid tbs: %s.",
					adapter_get_name(adap), sname, idx, p);
			return -1;
		}
		svr->tbs = svr->tbs * 1024;
		p = q + 1;
		LOGINFO("%s: Section %s: Loading server %d: tbs %d",
				adapter_get_name(adap), sname, idx, svr->tbs);

		/* flags */
		while (q && *p) {
			if ((q = strchr(p, ',')) != NULL)
				*q = 0;

			if (strcmp(p, "noheartbeat") == 0) {
				svr->flag |= CSSOCK_NO_HEARTBEAT;
				LOGINFO("%s: Section %s: Loading server %d: Flag: noheartbeat", adapter_get_name(adap), sname, idx);
			}
			else if (strcmp(p, "payload") == 0) {
				svr->flag |= CSSOCK_PAYLOAD_ONLY | CSSOCK_NO_HEARTBEAT;
				LOGINFO("%s: Section %s: Loading server %d: Flag: payload", adapter_get_name(adap), sname, idx);
			}
			else {
				LOGERROR4("%s: Section %s: Loading server %d: Invalid flag: %s.", adapter_get_name(adap), sname, idx, p);
				return -1;
			}
			p = q + 1;
		}
	}

	return 0;
}

static struct adapter_cs_ctl *adapter_cs_init(void *adap,
		unsigned long cfghd, char *sname, int (*running)(void), 
		int (*pktsync)(void *buf, int size))
{
	struct adapter_cs_ctl *ctl;
	struct adapter_cs_svrinfo *entry;

	ctl = (struct adapter_cs_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL)
		return NULL;
	memset(ctl, 0, sizeof(struct adapter_cs_ctl));
	INIT_LIST_HEAD(&(ctl->servers));

	if (adapter_cs_init_cfg(adap, ctl, cfghd, sname) < 0) {
		goto adapter_cs_init_failed;
	}

	ctl->mpktconn = mpktconn_open(running, pktsync);
	if (ctl->mpktconn == NULL) {
		LOGERROR("%s: Creating mpktconn failed.",
				adapter_get_name(adap));
		goto adapter_cs_init_failed;
	}
	//list_for_each_entry(entry, &(ctl->servers), lst) {
	for (entry = list_entry((&(ctl->servers))->next, struct adapter_cs_svrinfo, lst);
		 &entry->lst != (&(ctl->servers));
		 entry = list_entry(entry->lst.next, struct adapter_cs_svrinfo, lst))
	{
		if (pktsync)
			entry->flag |= CSSOCK_CUSTOMIZED_PKTSYNC;

		mpktconn_add_pktsvr_op(ctl->mpktconn, entry->ip, entry->port,
				entry->rbs, entry->tbs, entry->flag);
		LOGINFO("%s: Server %s@%d (%08x, Bufsz: Rx %dK, Tx %dK) added.",
				adapter_get_name(adap),
				entry->ip, entry->port, entry->flag,
				entry->rbs >> 10, entry->tbs >> 10);
	}

	return ctl;

adapter_cs_init_failed:
	adapter_cs_exit(ctl);
	return NULL;
}

static int adapter_cs_open(void *adap)
{
	struct adapter_cs_ctl *ctl;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (!ctl)
		return -1;

	if (ctl->mpktconn)
		mpktconn_start(ctl->mpktconn);

	return 0;
}

static void *adapter_cs_read(void *adap)
{
	struct adapter_cs_ctl *ctl;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (!ctl)
		return NULL;

	return mpktconn_read(ctl->mpktconn);
}

static void *adapter_cs_read_from(void *adap, char *ip, int *port)
{
	struct adapter_cs_ctl *ctl;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (!ctl)
		return NULL;

	return mpktconn_read_from(ctl->mpktconn, ip, port);
}

static int adapter_cs_write(void *adap, void *data, int len)
{
	struct adapter_cs_ctl *ctl;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (ctl) {
		return mpktconn_write(ctl->mpktconn, data, len);
	}

	return -1;
}

static int adapter_cs_write_to(void *adap, void *data, int len, char *ip, int port)
{
	struct adapter_cs_ctl *ctl;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (ctl) {
		return mpktconn_write_to(ctl->mpktconn, data, len, ip, port);
	}

	return -1;
}

static int adapter_cs_ioctl(void *adap, int code, void *arg)
{
	struct adapter_cs_ctl *ctl;
	int rc = -1;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (ctl) {
		switch (code) {
			case ADAPTER_CS_IOCTL_CLOSESOCKET:
				{
					struct adapter_cs_addr *addr;

					addr = (struct adapter_cs_addr *)arg;
					if (addr) {
						if (ctl->mpktconn) {
							mpktconn_close_svr(ctl->mpktconn, addr->ip, addr->port);
							rc = 0;
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

static void adapter_cs_close(void *adap)
{
	struct adapter_cs_ctl *ctl;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (ctl) {
		if (ctl->mpktconn)
			mpktconn_stop(ctl->mpktconn);
	}

	adapter_cs_exit(ctl);
}

static void *_adapter_register_cs(unsigned long cfghd, char *section,
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	void *adap;
	struct adapter_cs_ctl *ctl;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;
	adapter_set_name(adap, ADAPTER_CS_NAME);

	ctl = adapter_cs_init(adap, cfghd,
			section ? section: ADAPTER_CS_NAME, running, pktsync);
	if (ctl == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_data(adap, ctl);

	adapter_set_open(adap, adapter_cs_open);
	adapter_set_read(adap, adapter_cs_read);
	adapter_set_read_from(adap, adapter_cs_read_from);
	adapter_set_write(adap, adapter_cs_write);
	adapter_set_write_to(adap, adapter_cs_write_to);
	adapter_set_ioctl(adap, adapter_cs_ioctl);
	adapter_set_close(adap, adapter_cs_close);

	return adap;
}

void *adapter_register_cs(unsigned long cfghd, char *section,
		int (*running)(void))
{
	if (cfghd == 0ul)
		return NULL;

	return _adapter_register_cs(cfghd, section, running, NULL);
}

void *adapter_register_cs_cfgfile(char *cfgfile, char *section,
		int (*running)(void))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitialize(cfgfile);
	if (cfghd != 0ul) {
		p = _adapter_register_cs(cfghd, section, running, NULL);
		CfgInvalidate(cfghd);
	}

	return p;
}

void *adapter_register_cs_cfgstr(char *cfgstr, char *section,
		int (*running)(void))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitializeString(cfgstr);
	if (cfghd != 0ul) {
		p = _adapter_register_cs(cfghd, section, running, NULL);
		CfgInvalidate(cfghd);
	}

	return p;
}

void *adapter_register_cs_c(unsigned long cfghd, char *section,
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	if (cfghd == 0ul)
		return NULL;

	return _adapter_register_cs(cfghd, section, running, pktsync);
}

void *adapter_register_cs_cfgfile_c(char *cfgfile, char *section,
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitialize(cfgfile);
	if (cfghd != 0ul) {
		p = _adapter_register_cs(cfghd, section, running, pktsync);
		CfgInvalidate(cfghd);
	}

	return p;
}

void *adapter_register_cs_cfgstr_c(char *cfgstr, char *section,
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitializeString(cfgstr);
	if (cfghd != 0ul) {
		p = _adapter_register_cs(cfghd, section, running, pktsync);
		CfgInvalidate(cfghd);
	}

	return p;
}

void adapter_cs_setconncb(void *adap, CSSOCK_CONN_CB conncb, void *arg)
{
	struct adapter_cs_ctl *ctl;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (!ctl)
		return;

	if (ctl->mpktconn)
		mpktconn_set_conncb(ctl->mpktconn, conncb, arg);
}

void adapter_cs_setdisccb(void *adap, CSSOCK_DISC_CB disccb, void *arg)
{
	struct adapter_cs_ctl *ctl;

	ctl = (struct adapter_cs_ctl *)adapter_get_data(adap);
	if (!ctl)
		return;

	if (ctl->mpktconn)
		mpktconn_set_disccb(ctl->mpktconn, disccb, arg);
}

