/*
 *
 * pktcmd.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#else
#pragma warning ( disable : 4312 )
#include <time.h>
#endif
#include "os.h"
#include "aplog.h"
#include "hashtbl.h"
#include "pkt.h"
#include "pktcmd.h"
#include "pktcmdinfo.h"
#include "pktcmdhdl.h"
#include "adapter.h"
#include "misc.h"

struct pktcmd_ctl {
	void *ht;

	void *device;

	pktcmd_callback defcb;

	struct pktcmd_cmdinfo cmd;
};

static int pktcmd_ht_release(void *key, void *value)
{
	if (value)
		free(value);

	return 0;
}

static int pktcmd_ht_add(void *ht, struct pktcmd_cb_map *m)
{
	unsigned long key = (unsigned long)(m->id);
	struct pktcmd_cb_map *v;

	v = (struct pktcmd_cb_map *)malloc(sizeof(*v));
	if (v == NULL) {
		LOGERROR("Register CMD[0x%04x %s]: Failed, insufficient memory.",
				m->id, m->name);
		return -1;
	}
	memcpy(v, m, sizeof(*v));

	if (hashtbl_insert(ht, (void *)key, (void *)v) < 0) {
		LOGERROR("Register CMD[0x%04x %s]: Failed, hashtbl insert.",
				m->id, m->name);
		free(v);
		return -1;
	}

	return 0;
}

static struct pktcmd_cb_map *pktcmd_ht_query(void *ht, unsigned short id)
{
	unsigned long key = (unsigned long)id;

	return (struct pktcmd_cb_map *)hashtbl_find(ht, (void *)key, NULL);
}

static int pktcmd_default_callback(void *device, pktcmd_devcallback devcmdcb,
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len)
{
	if (cmd->type != PKT_TYPE_CMD) {
		LOGDEBUG("Processing CMD[0x%04x]: Silently discarding this %sACK.",
				cmd->id, (cmd->type == PKT_TYPE_CMDNACK) ? "N" : "");
	}
	else {
		LOGDEBUG("Processing CMD[0x%04x]: Echo payload back.", cmd->id);

		/* Echo this command packet back to the sender. */
		cmd->result = (unsigned char *)cmd->ph;
		cmd->size = pkthdr_get_plen(cmd->ph);
		cmd->ph = NULL;
	}

	return -1;
}

unsigned char *pktcmd_allocate_result(struct pktcmd_cmdinfo *cmd,
		unsigned int size)
{
	cmd->size = sizeof(pkt_hdr) + size;
	cmd->result = (unsigned char *)malloc(cmd->size);
	if (cmd->result == NULL) {
		LOGEMERGENCY("PKTCMD: Failed to allocate result buffer.");
		cmd->size = 0;
		return NULL;
	}
	memset(cmd->result, 0, cmd->size);

	return cmd->result + sizeof(pkt_hdr);
}

static int pktcmd_register_hdl(void *pc)
{
	struct pktcmd_ctl *ctl;
	struct pktcmd_cb_map *m, *p;

	ctl = (struct pktcmd_ctl *)pc;
	if (!ctl)
		return -1;

	for (m = pktcmd_cbs; m && m->id; m++) {
		p = pktcmd_ht_query(ctl->ht, m->id);
		if (p) {
			p->cmdcb = m->cmdcb;
			p->ackcb = m->ackcb;
			LOGWARN("Register CMD[0x%04x %s]: Already registered?",
					m->id, m->name);
		}
		else {
			pktcmd_ht_add(ctl->ht, m);
		}
	}

	return 0;
}

void *pktcmd_open(void *adap, void *device)
{
	struct pktcmd_ctl *ctl;

	ctl = (struct pktcmd_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL) {
		LOGERROR("pktcmd_open: Insufficient memory.");
		return NULL;
	}
	memset(ctl, 0, sizeof(*ctl));
	ctl->defcb = pktcmd_default_callback;

	ctl->ht = hashtbl_open(256, NULL, NULL, pktcmd_ht_release, "PktCMD");
	if (ctl->ht == NULL) {
		LOGERROR("pktcmd_open: Create hashtbl failed.");
		free(ctl);
		return NULL;
	}

	ctl->cmd.adap = adap;
	ctl->device = device;

	pktcmd_register_hdl(ctl);

	return ctl;
}

int pktcmd_register_hdl_ex(void *pc, void *cbmap)
{
	struct pktcmd_ctl *ctl;
	struct pktcmd_cb_map *cbs, *m, *p;

	ctl = (struct pktcmd_ctl *)pc;
	if (!ctl)
		return -1;

	cbs = (struct pktcmd_cb_map *)cbmap;
	if (!cbs)
		return -1;

	for (m = cbs; m && m->id; m++) {
		p = pktcmd_ht_query(ctl->ht, m->id);
		if (p) {
			p->cmdcb = m->cmdcb;
			p->ackcb = m->ackcb;
			LOGWARN("Register CMD[0x%04x %s]: Already registered?",
					m->id, m->name);
		}
		else {
			pktcmd_ht_add(ctl->ht, m);
		}
	}

	return 0;
}

int pktcmd_register_devhdl(void *pc, unsigned short id,
		pktcmd_devcallback cmdcb, pktcmd_devcallback ackcb)
{
	struct pktcmd_ctl *ctl;
	struct pktcmd_cb_map *p;

	ctl = (struct pktcmd_ctl *)pc;
	if (!ctl)
		return -1;

	p = pktcmd_ht_query(ctl->ht, id);
	if (!p) {
		LOGERROR("CMD[0x%04x]: Registering device callback failed.", id);
		return -1;
	}

	p->devcmdcb = cmdcb;
	p->devackcb = ackcb;

	return 0;
}

static int pktcmd_process_check_cmd(struct pktcmd_cmdinfo *cmd)
{
	unsigned char type, subtype;

	type = pkthdr_get_type(cmd->ph);
	subtype = pkthdr_get_subtype(cmd->ph);

	if (type == PKT_TYPE_KEEPALIVE)
		return -1;

	if (!TYPE_IS_CMD(type)) {
		LOGWARN("Processing CMD: %s@%d: Invalid type: 0x%02x",
				cmd->ip, cmd->port, type);
		return -1;
	}

	cmd->id = ((type & ~(PKT_TYPE_CMDACK | PKT_TYPE_CMDNACK)) << 8) | subtype;

	if (type & PKT_TYPE_CMDACK) {
		LOGDEBUG("Processing CMD[0x%04x]: This is an ACK from %s@%d"
				", %d payload bytes.",
				cmd->id, cmd->ip, cmd->port, pkthdr_get_dlen(cmd->ph));
		cmd->type = PKT_TYPE_CMDACK;
	}
	else if (type & PKT_TYPE_CMDNACK) {
		LOGDEBUG("Processing CMD[0x%04x]: This is an NACK from %s@%d"
				", %d payload bytes.",
				cmd->id, cmd->ip, cmd->port, pkthdr_get_dlen(cmd->ph));
		cmd->type = PKT_TYPE_CMDNACK;
	}
	else {
		LOGDEBUG("Processing CMD[0x%04x]: from %s@%d, %d payload bytes.",
				cmd->id, cmd->ip, cmd->port, pkthdr_get_dlen(cmd->ph));
		cmd->type = PKT_TYPE_CMD;
	}

	return 0;
}

/* Return value:
 *    1 - Not a command packet.
 *    2 - An ACK.
 *    3 - Successfully processed a command packet.
 *    0 - No command received.
 *   -1 - All other errors.
 */
int pktcmd_process(void *pc)
{
	int rc;
	struct pktcmd_ctl *ctl;
	struct pktcmd_cmdinfo *cmd;
	struct pktcmd_cb_map *v;

	ctl = (struct pktcmd_ctl *)pc;
	if (!ctl)
		return -1;
	cmd = &(ctl->cmd);

	cmd->ph = (pkt_hdr *)adapter_read_from(cmd->adap, cmd->ip, &(cmd->port));
	if (!(cmd->ph))
		return 0;

	if (pktcmd_process_check_cmd(cmd) < 0) {
		free(cmd->ph);
		cmd->ph = NULL;
		return 1;
	}

	cmd->result = NULL;
	cmd->size = 0;

	v = pktcmd_ht_query(ctl->ht, cmd->id);
	if (v == NULL) {
		rc = (*(ctl->defcb))(ctl->device, NULL,
				cmd, pkthdr_get_data(cmd->ph), pkthdr_get_dlen(cmd->ph));
	}
	else {
		if (cmd->type == PKT_TYPE_CMD) {
			rc = (*(v->cmdcb))(ctl->device, v->devcmdcb,
					cmd, pkthdr_get_data(cmd->ph), pkthdr_get_dlen(cmd->ph));
		}
		else {
			rc = (*(v->ackcb))(ctl->device, v->devackcb,
					cmd, pkthdr_get_data(cmd->ph), pkthdr_get_dlen(cmd->ph));
		}
	}
	if (cmd->ph) {
		free(cmd->ph);
		cmd->ph = NULL;
	}
	cmd->ack_type = ((rc < 0) ? PKT_TYPE_CMDNACK : PKT_TYPE_CMDACK);

	if (cmd->result) {
		struct timeval tv;

		pkthdr_set_sync((pkt_hdr *)(cmd->result));
		pkthdr_set_plen((pkt_hdr *)(cmd->result), cmd->size);
		pkthdr_set_type((pkt_hdr *)(cmd->result),
				((cmd->id >> 8) & 0x00f3) | cmd->ack_type);
		pkthdr_set_subtype((pkt_hdr *)(cmd->result), (cmd->id & 0x00ff));
		gettimeofday(&tv, NULL);
		pkthdr_set_ts((pkt_hdr *)(cmd->result), tv.tv_sec, tv.tv_usec * 1000);
		LOGDEBUG("Processing CMD[0x%04x]: Send %sACK to %s@%d:"
				" %d payload bytes.",
				cmd->id, (cmd->ack_type == PKT_TYPE_CMDNACK) ? "N" : "",
				cmd->ip, cmd->port,
				pkthdr_get_dlen((pkt_hdr *)(cmd->result)));

		adapter_write_to(cmd->adap, (pkt_hdr *)(cmd->result),
				cmd->size, cmd->ip, cmd->port);
		free(cmd->result);
		cmd->result = NULL;
		cmd->size = 0;
	}

	return 3;
}

int _pktcmd_sendto(void *pc, unsigned short id, unsigned char type,
		void *arg, int len, char *ip, int port)
{
	int enclen;
	struct pktcmd_ctl *ctl;
	struct pktcmd_cb_map *v;
	struct timeval tv;
	pkt_hdr *ph;

	ctl = (struct pktcmd_ctl *)pc;
	if (!ctl)
		return -1;

	if (!arg || (len < 1) ||
	    ((type != PKT_TYPE_CMD) && (type != PKT_TYPE_CMDACK) &&
	     (type != PKT_TYPE_CMDNACK)))
		return -1;

	v = pktcmd_ht_query(ctl->ht, id);
	if (v == NULL) {
		LOGERROR("CMD[0x%04x]: Unregistered, unable to send it.", id);
		return -1;
	}

	ph = (pkt_hdr *)malloc(sizeof(pkt_hdr) + len + 64);
	if (ph == NULL) {
		LOGERROR("CMD[0x%04x %s]: sending: insufficient memory (%d bytes).",
				v->id, v->name, len);
		return -1;
	}
	memset(ph, 0, sizeof(pkt_hdr) + len + 64);

	enclen = -1;
	if (type == PKT_TYPE_CMD) {
		if (v->inenc == NULL)
			enclen = 0;
		else if (arg != NULL)
			enclen = (*(v->inenc))(pkthdr_get_data(ph), len + 64, arg);
	}
	else {
		if (v->outenc == NULL)
			enclen = 0;
		else if (arg != NULL)
			enclen = (*(v->outenc))(pkthdr_get_data(ph), len + 64, arg);
	}
	if (enclen < 0) {
		LOGERROR("CMD[0x%04x %s]: sending%s: encoding args failed.",
				v->id, v->name, ((type == PKT_TYPE_CMD) ? "" : (
						(type == PKT_TYPE_CMDACK) ? " ACK" : " NACK")));
		free(ph);
		return -1;
	}

	pkthdr_set_sync(ph);
	pkthdr_set_dlen(ph, enclen);
	pkthdr_set_type(ph, ((id >> 8) & 0xff) | type);
	pkthdr_set_subtype(ph, id & 0xff);
	pkthdr_set_device(ph, 0);
	pkthdr_set_channel(ph, 0);
	gettimeofday(&tv, NULL);
	pkthdr_set_ts(ph, tv.tv_sec, tv.tv_usec * 1000);

	LOGDEBUG("CMD[0x%04x %s]: sending%s: %d payload bytes sent to %s@%d.",
			v->id, v->name, ((type == PKT_TYPE_CMD) ? "" : (
					(type == PKT_TYPE_CMDACK) ? " ACK" : " NACK")),
			enclen, ip ? ip : "ALL", port);
	adapter_write_to(ctl->cmd.adap, ph, pkthdr_get_plen(ph), ip, port);
	free(ph);

	return 0;
}

int pktcmd_sendto(void *pc, unsigned short id, void *in, int len,
		char *ip, int port)
{
	return _pktcmd_sendto(pc, id, PKT_TYPE_CMD, in, len, ip, port);
}

int pktcmd_send(void *pc, unsigned short id, void *in, int len)
{
	return _pktcmd_sendto(pc, id, PKT_TYPE_CMD, in, len, NULL, 0);
}

int pktcmd_sendackto(void *pc, unsigned short id, void *out, int len,
		char *ip, int port)
{
	return _pktcmd_sendto(pc, id, PKT_TYPE_CMDACK, out, len, ip, port);
}

int pktcmd_sendack(void *pc, unsigned short id, void *out, int len)
{
	return _pktcmd_sendto(pc, id, PKT_TYPE_CMDACK, out, len, NULL, 0);
}

int pktcmd_sendnackto(void *pc, unsigned short id, void *out, int len,
		char *ip, int port)
{
	return _pktcmd_sendto(pc, id, PKT_TYPE_CMDNACK, out, len, ip, port);
}

int pktcmd_sendnack(void *pc, unsigned short id, void *out, int len)
{
	return _pktcmd_sendto(pc, id, PKT_TYPE_CMDNACK, out, len, NULL, 0);
}

void pktcmd_close(void *pc)
{
	struct pktcmd_ctl *ctl;

	ctl = (struct pktcmd_ctl *)pc;
	if (ctl) {
		if (ctl->ht)
			hashtbl_close(ctl->ht);
		free(ctl);
	}
}
