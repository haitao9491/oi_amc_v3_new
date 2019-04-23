/*
 *
 * mpktconn.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#else
#pragma warning( disable : 4996 )
#endif
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "cthread.h"
#include "list.h"
#include "pkt.h"
#include "pktqueue.h"
#include "csock.h"
#include "mpktconn.h"

struct mpktconn_hd {
	int (*check_running)(void);
	int (*pktsync)(void *buf, int size);

	CSSOCK_CONN_CB conncb;
	void	      *connarg;
	CSSOCK_DISC_CB disccb;
	void	      *discarg;

	int rbs; /* Receive buffer size of underlying socket layer  */
	int tbs; /* Transmit buffer size of underlying socket layer */

	unsigned int flag;

	struct list_head svr;
};

struct mpktconn_svr_info {
	struct list_head node;

	char ip[64];
	int  port;
	struct mpktconn_hd *mhd;

	unsigned int flag;

	void      *cs;
	int        rbs;
	int        tbs;
	void      *docheck;
	void      *doflush_rx;
	void      *doflush_tx;
};

void *mpktconn_do_check(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	struct mpktconn_svr_info *svrinfo;
	pkt_hdr hbpkt;
	int counter = 0, max = 5;
	int i = 0;

	svrinfo = (struct mpktconn_svr_info *)targ->arg;
	if (!svrinfo || !svrinfo->cs) {
		LOGERROR("mpktconn: connection to %s@%d: No connection.",
				svrinfo->ip, svrinfo->port);
		return NULL;
	}

	memset(&hbpkt, 0, sizeof(hbpkt));
	pkthdr_set_sync(&hbpkt);
	pkthdr_set_dlen(&hbpkt, 0);
	pkthdr_set_type(&hbpkt, PKT_TYPE_HEARTBEAT);
	pkthdr_set_subtype(&hbpkt, PKT_SUBTYPE_HEARTBEAT_SOCKET);

	while (svrinfo->mhd->check_running() && targ->flag) {
		if (counter >= max) {
			if (csock_check_connection(svrinfo->cs) == 1) {
				if ((svrinfo->flag & CSSOCK_NO_HEARTBEAT) != CSSOCK_NO_HEARTBEAT) {
					csock_write(svrinfo->cs, (char *)&hbpkt, sizeof(hbpkt));
				}
				max = 50;
			}
			else {
				max = 5;
			}
			counter = 0;
		}

		counter++;
		SLEEP_US(100000);

		i++;
		if ((i & 0x0fff) == 0) {
			/* Output the socket level statistics about every 410 seconds */
			csock_stat(svrinfo->cs);
		}
	}

	csock_stat(svrinfo->cs);
	LOGINFO("mpktconn: connection to %s@%d: thread (check) terminated.",
			svrinfo->ip, svrinfo->port);
	return NULL;
}

void *mpktconn_do_flush_rx(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	struct mpktconn_svr_info *svrinfo;

	svrinfo = (struct mpktconn_svr_info *)targ->arg;

	if (!svrinfo || !svrinfo->cs) {
		LOGERROR("mpktconn: connection to %s@%d: No connection.",
				svrinfo->ip, svrinfo->port);
		return NULL;
	}

	while (svrinfo->mhd->check_running() && targ->flag) {
		if (csock_flush_rx(svrinfo->cs) <= 0) {
			SLEEP_US(1000);
		}
	}

	LOGINFO("mpktconn: connection to %s@%d: thread (flush_rx) terminated.",
			svrinfo->ip, svrinfo->port);
	return NULL;
}

void *mpktconn_do_flush_tx(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	struct mpktconn_svr_info *svrinfo;

	svrinfo = (struct mpktconn_svr_info *)targ->arg;

	if (!svrinfo || !svrinfo->cs) {
		LOGERROR("mpktconn: connection to %s@%d: No connection.",
				svrinfo->ip, svrinfo->port);
		return NULL;
	}

	while (svrinfo->mhd->check_running() && targ->flag) {
		if (csock_flush_tx(svrinfo->cs) <= 0) {
#if defined(__tilegx__)
			SLEEP_US(100);
#else
			SLEEP_US(1000);
#endif
		}
	}

	LOGINFO("mpktconn: connection to %s@%d: thread (flush_tx) terminated.",
			svrinfo->ip, svrinfo->port);
	return NULL;
}

void *mpktconn_open(int (*check_running)(void), int (*pktsync)(void *buf, int size))
{
	struct mpktconn_hd *hd;

	if (!check_running)
		return NULL;

	hd = (struct mpktconn_hd *)malloc(sizeof(struct mpktconn_hd));
	if (hd == NULL)
		return NULL;

	memset(hd, 0, sizeof(struct mpktconn_hd));

	hd->check_running = check_running;
	hd->pktsync = pktsync;
	hd->rbs = 4 * 1024 * 1024;
	hd->tbs = 4 * 1024 * 1024;
	hd->flag = 0;
	INIT_LIST_HEAD(&(hd->svr));

	return hd;
}

int mpktconn_add_pktsvr_op(void *hd, char *ip, int port,
		int rbs, int tbs, unsigned int flag)
{
	struct mpktconn_hd       *mhd;
	struct list_head         *pos;
	struct mpktconn_svr_info *entry;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd || !ip || (strlen(ip) > 63) || (port < 1) ||
			(rbs < 0) || (tbs < 0))
		return -1;

	list_for_each(pos, &(mhd->svr)) {
		entry = list_entry(pos, struct mpktconn_svr_info, node);
		if ((strcmp(ip, entry->ip) == 0) && (port == entry->port))
			return -1;
	}

	entry = (struct mpktconn_svr_info *)malloc(sizeof(*entry));
	if (entry == NULL)
		return -1;
	memset(entry, 0, sizeof(*entry));

	strcpy(entry->ip, ip);
	entry->port = port;
	entry->mhd = mhd;
	entry->flag = flag;
	entry->rbs = rbs;
	entry->tbs = tbs;
	entry->docheck = NULL;
	entry->doflush_rx = NULL;
	entry->doflush_tx = NULL;

	list_add_tail(&(entry->node), &(mhd->svr));

	return 0;
}

int mpktconn_add_pktsvr(void *hd, char *ip, int port)
{
	struct mpktconn_hd *mhd;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd)
		return -1;

	return mpktconn_add_pktsvr_op(hd, ip, port,
			mhd->rbs, mhd->tbs, mhd->flag);
}

int mpktconn_set_op(void *hd, int rbs, int tbs, unsigned int flag)
{
	struct mpktconn_hd *mhd;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd || (rbs < 1024) || (tbs < 1024))
		return -1;

	mhd->rbs = rbs;
	mhd->tbs = tbs;

	mhd->flag = flag;

	return 0;
}

int mpktconn_start(void *hd)
{
	struct mpktconn_hd       *mhd;
	struct list_head         *pos;
	struct mpktconn_svr_info *entry;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd)
		return -1;

	list_for_each(pos, &(mhd->svr)) {
		entry = list_entry(pos, struct mpktconn_svr_info, node);
		if (entry->cs)
			continue;

		entry->cs = csock_open(entry->ip, entry->port,
					entry->rbs, entry->tbs, mhd->pktsync);
		if (entry->cs == NULL) {
			LOGERROR("mpktconn: open csocket to %s@%d failed.",
					entry->ip, entry->port);
			continue;
		}
		csock_setflag(entry->cs, entry->flag);
		csock_setconncb(entry->cs, mhd->conncb, mhd->connarg);
		csock_setdisccb(entry->cs, mhd->disccb, mhd->discarg);

		entry->docheck = thread_open(
				(LPTHREAD_START_ROUTINE)mpktconn_do_check, (void *)entry);
		if (entry->docheck == NULL) {
			LOGERROR("mpktconn: connection to %s@%d: "
					"Failed starting thread (check).", entry->ip, entry->port);
		}
		LOG("mpktconn: connection to %s@%d: thread (check) started.",
				entry->ip, entry->port);

		entry->doflush_rx = thread_open(
				(LPTHREAD_START_ROUTINE)mpktconn_do_flush_rx, (void *)entry);
		if (entry->doflush_rx == NULL) {
			LOGERROR("mpktconn: connection to %s@%d: "
					"Failed starting thread (flush_rx).",
					entry->ip, entry->port);
		}
		LOG("mpktconn: connection to %s@%d: thread (flush_rx) started.",
				entry->ip, entry->port);

		entry->doflush_tx = thread_open(
				(LPTHREAD_START_ROUTINE)mpktconn_do_flush_tx, (void *)entry);
		if (entry->doflush_tx == NULL) {
			LOGERROR("mpktconn: connection to %s@%d: "
					"Failed starting thread (flush_tx).",
					entry->ip, entry->port);
		}
		LOG("mpktconn: connection to %s@%d: thread (flush_tx) started.",
				entry->ip, entry->port);
	}

	return 0;
}

void *mpktconn_read_from(void *hd, char *ip, int *port)
{
	struct mpktconn_hd       *mhd;
	struct list_head         *pos;
	struct mpktconn_svr_info *entry;
	void *ph;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd)
		return NULL;

	list_for_each(pos, &(mhd->svr)) {
		entry = list_entry(pos, struct mpktconn_svr_info, node);
		if (!entry->cs)
			continue;

		if ((ph = csock_read(entry->cs)) != NULL) {
			if (ip)
				strcpy(ip, entry->ip);
			if (port)
				*port = entry->port;
			return ph;
		}
	}

	return NULL;
}

void *mpktconn_read(void *hd)
{
	return mpktconn_read_from(hd, NULL, NULL);
}

int mpktconn_write_to(void *hd, char *data, int len, char *ip, int port)
{
	struct mpktconn_hd       *mhd;
	struct list_head         *pos;
	struct mpktconn_svr_info *entry;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd)
		return -1;

	if (list_empty(&(mhd->svr)))
		return -1;

	list_for_each(pos, &(mhd->svr)) {
		entry = list_entry(pos, struct mpktconn_svr_info, node);
		if (!entry->cs)
			continue;

		if (port && (port != entry->port))
			continue;

		if (ip && strcmp(ip, entry->ip))
			continue;

		csock_write(entry->cs, data, len);
	}

	return len;
}

int mpktconn_write(void *hd, char *data, int len)
{
	return mpktconn_write_to(hd, data, len, NULL, 0);
}

void mpktconn_stop(void *hd)
{
	struct mpktconn_hd       *mhd;
	struct list_head         *pos;
	struct mpktconn_svr_info *entry;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd)
		return;

	list_for_each(pos, &(mhd->svr)) {
		entry = list_entry(pos, struct mpktconn_svr_info, node);
		if (!entry->cs)
			continue;

		if (entry->doflush_rx) {
			thread_close(entry->doflush_rx);
			entry->doflush_rx = NULL;
		}

		if (entry->doflush_tx) {
			thread_close(entry->doflush_tx);
			entry->doflush_tx = NULL;
		}

		if (entry->docheck) {
			thread_close(entry->docheck);
			entry->docheck = NULL;
		}

		csock_close(entry->cs);
		entry->cs = NULL;
	}
}

void mpktconn_close(void *hd)
{
	struct mpktconn_hd       *mhd;
	struct list_head         *pos, *n;
	struct mpktconn_svr_info *entry;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd)
		return;

	list_for_each_safe(pos, n, &(mhd->svr)) {
		entry = list_entry(pos, struct mpktconn_svr_info, node);
		list_del(pos);
		free(entry);
	}

	free(mhd);
}

void mpktconn_close_svr(void *hd, char *ip, int port)
{
	struct mpktconn_hd       *mhd;
	struct list_head         *pos;
	struct mpktconn_svr_info *entry;

	mhd = (struct mpktconn_hd *)hd;
	if (!mhd)
		return;

	list_for_each(pos, &(mhd->svr)) {
		entry = list_entry(pos, struct mpktconn_svr_info, node);
		if ((strcmp(ip, entry->ip) == 0) && (port == entry->port)) {
			csock_close_socket_lock(entry->cs);
			return;
		}
	}
}
        
void mpktconn_set_conncb(void *hd, CSSOCK_CONN_CB conncb, void *arg)
{
	struct mpktconn_hd *mhd = (struct mpktconn_hd *)hd;

	if (mhd) {
		mhd->conncb = conncb;
		mhd->connarg = arg;
	}
}

void mpktconn_set_disccb(void *hd, CSSOCK_DISC_CB disccb, void *arg)
{
	struct mpktconn_hd *mhd = (struct mpktconn_hd *)hd;

	if (mhd) {
		mhd->disccb = disccb;
		mhd->discarg = arg;
	}
}

