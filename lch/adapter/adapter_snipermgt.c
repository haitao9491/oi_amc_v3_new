/*
 * (C) Copyright 2013
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * adapter_snipermgt.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "os.h"
#include "aplog.h"
#include "cthread.h"
#include "cconfig.h"
#include "list.h"
#include "pkt.h"
#include "pktqueue.h"
#include "hash.h"
#include "hashtbl.h"
#include "misc.h"
#include "utimer.h"
#include "cssockdef.h"
#include "adapter.h"
#include "adapter_ss.h"
#include "adapter_snipermgt.h"

#define ADAPTER_SNIPERMGT_NAME "Adapter.SNIPERMGTServerSocket"

struct adapter_snipermgt_ctl {
	void *ss;    /* Adapter Server Socket */
	void *ht;    /* Buffer to recover SNIPERMGT packets, organized by connection */
	void *pktq;  /* Packet queue */

	int (*running)(void);

	void *dorecv;
	void *timer;
};

struct adapter_snipermgt_upkt {
	unsigned int ip;
	unsigned int port;
	pkt_hdr     *ph;
};

struct snipermgt_ht_key {
	unsigned int ip;
	unsigned int port;
};

struct snipermgt_ht_value {
	unsigned char *buf;
	int bufsz;
	int datapos, datalen;

	unsigned long long discarded_byte;
	unsigned long long dontcare_byte;
	unsigned long long ok_byte;
	unsigned long long ok_pkt;
};


static unsigned long snipermgt_ht_hash(void *key, unsigned int bits)
{
	struct snipermgt_ht_key *k = (struct snipermgt_ht_key *)key;

	return hash_long((unsigned long)((k->ip << 16) | (k->port & 0xffff)),
			bits);
}

static int snipermgt_ht_key_cmp(void *a, void *b)
{
	struct snipermgt_ht_key *ka = (struct snipermgt_ht_key *)a;
	struct snipermgt_ht_key *kb = (struct snipermgt_ht_key *)b;

	if ((ka->ip == kb->ip) && (ka->port == kb->port))
		return 0;

	return 1;
}

static int snipermgt_ht_release(void *key, void *value)
{
	if (key)
		free(key);
	if (value) {
		struct snipermgt_ht_value *v = (struct snipermgt_ht_value *)value;
		free(v->buf);
		free(v);
	}

	return 0;
}

static struct snipermgt_ht_value *snipermgt_ht_find_and_insert(void *ht,
		char *ip, int port)
{
	struct in_addr t;
	struct snipermgt_ht_key k, *pk;
	struct snipermgt_ht_value *v;

	if (inet_aton(ip, &t))
		k.ip = (unsigned int)t.s_addr;
	else
		k.ip = 0;
	k.port = port;

	v = (struct snipermgt_ht_value *)hashtbl_find(ht, &k, NULL);
	if (v)
		return v;

	pk = (struct snipermgt_ht_key *)malloc(sizeof(*pk));
	if (pk == NULL)
		return NULL;
	pk->ip = k.ip;
	pk->port = k.port;

	v = (struct snipermgt_ht_value *)malloc(sizeof(*v));
	if (v == NULL) {
		free(pk);
		return NULL;
	}
	memset(v, 0, sizeof(*v));

	v->bufsz = 384 * 1024;
	v->buf = (unsigned char *)malloc(v->bufsz);
	if (v->buf == NULL) {
		free(pk);
		free(v);
		return NULL;
	}

	if (hashtbl_insert(ht, pk, v) < 0) {
		free(pk);
		free(v->buf);
		free(v);
		return NULL;
	}

	return v;
}

static void adapter_snipermgt_exit_clear_pktq(void *pktq)
{
	struct adapter_snipermgt_upkt *upkt = NULL;

	for (;;) {
		upkt = (struct adapter_snipermgt_upkt *)pkt_pop(pktq);
		if (upkt == NULL)
			break;

		if (upkt->ph)
			free(upkt->ph);
		free(upkt);
	}

	pkt_queue_close(pktq);
}

static void adapter_snipermgt_exit(struct adapter_snipermgt_ctl *ctl)
{
	if (ctl) {
		if (ctl->ss)
			adapter_close(ctl->ss);
		if (ctl->ht)
			hashtbl_close(ctl->ht);
		if (ctl->pktq)
			adapter_snipermgt_exit_clear_pktq(ctl->pktq);
		if (ctl->timer)
			utimer_close(ctl->timer);

		free(ctl);
	}
}

void adapter_snipermgt_ht_traverse(void *key, void *value, void *arg)
{
	struct snipermgt_ht_key *k = (struct snipermgt_ht_key *)key;
	struct snipermgt_ht_value *v = (struct snipermgt_ht_value *)value;

	LOG("Statistics of %u.%u.%u.%u@%u: %llu (%llu discarded, %llu don't care, "
			"%llu processed)",
			(k->ip >> 24) & 0xff, (k->ip >> 16) & 0xff,
			(k->ip >> 8) & 0xff, (k->ip >> 0) & 0xff, k->port,
			v->discarded_byte + v->dontcare_byte + v->ok_byte,
			v->discarded_byte, v->dontcare_byte, v->ok_byte);
}

void adapter_snipermgt_timer_callback(unsigned int s, unsigned int ns, void *data)
{
	struct adapter_snipermgt_ctl *ctl;
	static int counter = 0;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(data);
	if (ctl == NULL)
		return;

	if (++counter == 300) {
		hashtbl_traverse(ctl->ht, adapter_snipermgt_ht_traverse, NULL);
		counter = 0;
	}

	utimer_add_by_offset(ctl->timer, 1, 0, data);
}

static struct adapter_snipermgt_ctl *adapter_snipermgt_init(void *adap,
		unsigned long cfghd, char *sname, int (*running)(void))
{
	struct adapter_snipermgt_ctl *ctl;

	ctl = (struct adapter_snipermgt_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL)
		return NULL;
	memset(ctl, 0, sizeof(struct adapter_snipermgt_ctl));

	ctl->running = running;
	ctl->dorecv = NULL;

	ctl->timer = utimer_open(UTIMER_INTERNAL_TIMESOURCE, 
			adapter_snipermgt_timer_callback);
	if (ctl->timer == NULL)
		goto adapter_snipermgt_init_failed;
	utimer_add_by_offset(ctl->timer, 1, 0, adap);

	ctl->pktq = pkt_queue_open(0, "PKTQ.SNIPERMGT");
	if (ctl->pktq == NULL)
		goto adapter_snipermgt_init_failed;

	ctl->ht = hashtbl_open(16,
			snipermgt_ht_hash, snipermgt_ht_key_cmp, snipermgt_ht_release,
			"Adapter.SNIPERMGT");
	if (ctl->ht == NULL)
		goto adapter_snipermgt_init_failed;

	ctl->ss = adapter_register_ss(cfghd, sname, running);
	if (ctl->ss == NULL)
		goto adapter_snipermgt_init_failed;

	return ctl;

adapter_snipermgt_init_failed:
	adapter_snipermgt_exit(ctl);
	return NULL;
}

static int adapter_snipermgt_pkt_verify(unsigned char *buf, int size)
{
	int len;
	unsigned char cmd;

	if (size < 5)
		return 0;
	size -= 5;

	cmd = *buf;
	if ((cmd != 0x01) && (cmd != 0x02) && (cmd != 0x03) && (cmd != 0x04))
		return -1;

	len = (*(buf + 3) << 8) | *(buf + 4);
	if (len <= size)
		return len;

	return 0;
}

static void adapter_snipermgt_process(struct adapter_snipermgt_ctl *ctl,
		pkt_hdr *ph, char *ip, int port)
{
	int len, nlen;
	int error = 0;
	struct snipermgt_ht_value *v;
	struct adapter_snipermgt_upkt *upkt;

	v = snipermgt_ht_find_and_insert(ctl->ht, ip, port);
	if (v == NULL) {
		LOGERROR("Unable to locate internal buffer: %s@%d", ip, port);
		return;
	}

	if ((v->datapos > 0) && (v->datalen > 0)) {
		memcpy(v->buf, v->buf + v->datapos, v->datalen);
	}
	v->datapos = 0;

	nlen = pkthdr_get_dlen(ph);
	if (nlen + v->datalen > v->bufsz) {
		LOGERROR("Too many unaligned data stuck in buffer: %s@%d: %d, "
				"%d newly arrived. (DISCARDED)",
				ip, port, v->datalen, nlen);
		v->discarded_byte += (nlen + v->datalen);
		v->datalen = 0;
		return;
	}
	memcpy(v->buf + v->datalen, pkthdr_get_data(ph), nlen);
	v->datalen += nlen;

	while ((len = adapter_snipermgt_pkt_verify(v->buf + v->datapos, v->datalen)) != 0) {
		/* try to recovery */
		if (len < 0) {
			v->datapos++;
			v->datalen--;
			v->discarded_byte++;
			error = 1;
			continue;
		}

		/* process message */
		upkt = (struct adapter_snipermgt_upkt *)malloc(sizeof(*upkt));
		if (upkt == NULL) {
			v->datapos += (len + 5);
			v->datalen -= (len + 5);
			v->discarded_byte += (len + 5);
			continue;
		}
		upkt->ph = (pkt_hdr *)malloc(len + 5);
		if (upkt->ph == NULL) {
			free(upkt);

			v->datapos += (len + 5);
			v->datalen -= (len + 5);
			v->discarded_byte += (len + 5);
			continue;
		}

		upkt->ip = inet_addr(ip);
		upkt->port = port;
		memcpy(upkt->ph, v->buf + v->datapos, len + 5);

		if (pkt_push(ctl->pktq, (void *)upkt, sizeof(*upkt)) <= 0) {
			free(upkt->ph);
			free(upkt);
		}

		v->ok_pkt++;
		v->ok_byte += (len + 5);

		v->datapos += (len + 5);
		v->datalen -= (len + 5);
	}

	if (error) {
		LOGERROR("Unaligned data found: %s@%d: "
				"Now total %llu bytes discarded.",
				ip, port, v->discarded_byte);
	}
}

static void *adapter_snipermgt_do_recv(void *arg)
{
	char ip[32];
	int  port;
	pkt_hdr *ph;
	struct adapter_snipermgt_ctl *ctl;
	struct thread_arg *targ = (struct thread_arg *)arg;
	void              *adap = targ->arg;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (!ctl)
		return NULL;

	while (ctl->running()) {
		ph = (pkt_hdr *)adapter_read_from(ctl->ss, ip, &port);
		if (ph == NULL) {
			utimer_run(ctl->timer, 0, 0);
			usleep(1000);
			continue;
		}

		adapter_snipermgt_process(ctl, ph, ip, port);
		free(ph);
	}

	LOG("%s: Thread (recv) terminated.", adapter_get_name(adap));
	return NULL;
}

static int adapter_snipermgt_open(void *adap)
{
	struct adapter_snipermgt_ctl *ctl;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (!ctl)
		return -1;

	adapter_open(ctl->ss);

	if (ctl->dorecv == NULL) {
		ctl->dorecv = thread_open(adapter_snipermgt_do_recv, adap);
		if (ctl->dorecv == NULL) {
			LOGERROR("%s: Failed creating thread (recv).",
					adapter_get_name(adap));
			return -1;
		}
		LOG("%s: Thread (recv) started.", adapter_get_name(adap));
	}

	return 0;
}

static void *adapter_snipermgt_read(void *adap)
{
	struct adapter_snipermgt_ctl *ctl;
	struct adapter_snipermgt_upkt *upkt;
	pkt_hdr *ph = NULL;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (!ctl || !ctl->pktq)
		return NULL;

	upkt = (struct adapter_snipermgt_upkt *)pkt_pop(ctl->pktq);
	if (upkt) {
		ph = upkt->ph;
		free(upkt);
	}

	return ph;
}

static void *adapter_snipermgt_read_from(void *adap, char *ip, int *port)
{
	struct adapter_snipermgt_ctl *ctl;
	struct adapter_snipermgt_upkt *upkt;
	pkt_hdr *ph = NULL;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (!ctl || !ctl->pktq)
		return NULL;

	upkt = (struct adapter_snipermgt_upkt *)pkt_pop(ctl->pktq);
	if (upkt) {
		ph = upkt->ph;

		if (ip)
			ip4addr_str(upkt->ip, ip);
		if (port)
			*port = ntohs(upkt->port);

		free(upkt);
	}

	return ph;
}

static int adapter_snipermgt_write(void *adap, void *data, int len)
{
	struct adapter_snipermgt_ctl *ctl;
	unsigned char  buf[8192];
	pkt_hdr       *ph;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (!ctl || !data || (len <= 0))
		return -1;

	if (len > (8192 - 20)) {
		LOGERROR("%s: insufficent internal buffer.", adapter_get_name(adap));
		return -1;
	}

	memset(buf, 0, 20);
	ph = (pkt_hdr *)buf;

	pkthdr_set_sync(ph);
	pkthdr_set_dlen(ph, len);
	memcpy(buf + sizeof(*ph), data, len);

	return adapter_write(ctl->ss, buf, pkthdr_get_plen(ph));
}

static int adapter_snipermgt_write_to(void *adap, void *data, int len, char *ip, int port)
{
	struct adapter_snipermgt_ctl *ctl;
	unsigned char  buf[8192];
	pkt_hdr       *ph;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (!ctl || !data || (len <= 0))
		return -1;

	if (len > (8192 - 20)) {
		LOGERROR("%s: insufficent internal buffer.", adapter_get_name(adap));
		return -1;
	}

	memset(buf, 0, 20);
	ph = (pkt_hdr *)buf;

	pkthdr_set_sync(ph);
	pkthdr_set_dlen(ph, len);
	memcpy(buf + sizeof(*ph), data, len);

	return adapter_write_to(ctl->ss, buf, pkthdr_get_plen(ph), ip, port);
}

static void adapter_snipermgt_close(void *adap)
{
	struct adapter_snipermgt_ctl *ctl;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (ctl) {
		if (ctl->dorecv) {
			thread_close(ctl->dorecv);
			ctl->dorecv = NULL;
			LOG("%s: Thread (recv) terminated.", adapter_get_name(adap));
		}
	}

	adapter_snipermgt_exit(ctl);
}

DLL_APP void *adapter_register_snipermgt(unsigned long cfghd, char *section, int (*running)(void))
{
	void *adap;
	struct adapter_snipermgt_ctl *ctl;

	if (!running)
		return NULL;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;
	adapter_set_name(adap, ADAPTER_SNIPERMGT_NAME);

	ctl = adapter_snipermgt_init(adap, cfghd,
			section ? section : (char *)ADAPTER_SNIPERMGT_NAME,running);
	if (ctl == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_data(adap, ctl);

	adapter_set_open(adap, adapter_snipermgt_open);
	adapter_set_read(adap, adapter_snipermgt_read);
	adapter_set_read_from(adap, adapter_snipermgt_read_from);
	adapter_set_write(adap, adapter_snipermgt_write);
	adapter_set_write_to(adap, adapter_snipermgt_write_to);
	adapter_set_close(adap, adapter_snipermgt_close);

	return adap;
}

DLL_APP void *adapter_register_snipermgt_cfgfile(char *cfgfile, char *section, int (*running)(void))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitialize(cfgfile);
	if (cfghd != 0ul) {
		p = adapter_register_snipermgt(cfghd, section, running);
		CfgInvalidate(cfghd);
	}

	return p;
}

DLL_APP void *adapter_register_snipermgt_cfgstr(char *cfgstr, char *section, int (*running)(void))
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitializeString(cfgstr);
	if (cfghd != 0ul) {
		p = adapter_register_snipermgt(cfghd, section, running);
		CfgInvalidate(cfghd);
	}

	return p;
}

DLL_APP void adapter_snipermgt_setconncb(void *adap, CSSOCK_CONN_CB conncb, void *arg)
{
	struct adapter_snipermgt_ctl *ctl;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (ctl && ctl->ss)
		adapter_ss_setconncb(ctl->ss, conncb, arg);
}

DLL_APP void adapter_snipermgt_setdisccb(void *adap, CSSOCK_DISC_CB disccb, void *arg)
{
	struct adapter_snipermgt_ctl *ctl;

	ctl = (struct adapter_snipermgt_ctl *)adapter_get_data(adap);
	if (ctl && ctl->ss)
		adapter_ss_setdisccb(ctl->ss, disccb, arg);
}

