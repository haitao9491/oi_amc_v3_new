/*
 *
 * nm_glb.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "os.h"
#include "aplog.h"
#include "list.h"
#include "mutex.h"
#include "pkt.h"
#include "nmpkt.h"
#include "nm_typdef.h"
#include "coding.h"
#include "nm_glb.h"

struct nms_bdinfo {
	char ip[IP_BUF_LEN];
	int  port;
	struct nmboard_info bdi;
	struct list_head node;
};

struct nms_glb_ctl {
	int nmbdi_cnt;
	struct list_head lsthd;   /* nms_bdinfo */
	void  *lock;
};

void *nms_glb_open(void)
{
	struct nms_glb_ctl *ctl = NULL;
	
	ctl = (struct nms_glb_ctl *)malloc(sizeof(*ctl));
	if (!ctl) {
		LOGERROR("%s: malloc failed!", __func__);
		return NULL;
	}

	memset(ctl, 0, sizeof(*ctl));
	INIT_LIST_HEAD(&ctl->lsthd);

	ctl->lock = mutex_open(NULL);
	if (!ctl->lock) {
		LOGERROR("%s: mutex_open failed!", __func__);
		free(ctl);
		return NULL;
	}

	return ctl;
}

static void *nms_glb_find_entry_byip(void *hd, char *ip, int port)
{
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;
	struct list_head *p = NULL;
	struct nms_bdinfo *entry = NULL;

	if (NULL != hd && NULL != ip) {
		list_for_each(p, &ctl->lsthd) {
			entry = list_entry(p, struct nms_bdinfo, node);
			if (NULL != entry && !strcmp(ip, entry->ip) && port == entry->port) {
				return entry;
			}
		}
	}

	return NULL;
}

static void *nms_glb_find_entry_bylki(void *hd, struct nm_lkaddr_info *lki)
{
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;
	struct list_head *p = NULL;
	struct nms_bdinfo *entry = NULL;

	if (NULL != hd && NULL != lki) {

		list_for_each(p, &ctl->lsthd) {
			entry = list_entry(p, struct nms_bdinfo, node);
			if (NULL != entry && entry->bdi.rack == lki->rack && 
					entry->bdi.shelf == lki->shelf && 
					entry->bdi.slot == lki->slot &&
					entry->bdi.subslot == lki->subslot)
			{
				return entry;
			}
		}
	}

	return NULL;
}

int nms_glb_insert_bdinfo(void *hd, char *ip, int port, struct nmboard_info *bdi)
{
	int ret = -1;
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;
	struct nms_bdinfo *entry = NULL;

	if (!ctl && !ip)
		return -1;

	mutex_lock(ctl->lock);
	entry = nms_glb_find_entry_byip(ctl, ip, port);
	if (NULL != entry) {
		if (NULL != bdi)
			memcpy(&entry->bdi, bdi, sizeof(entry->bdi));

		ret = 0;
	}
	else {
		entry = malloc(sizeof(*entry));
		if (NULL != entry) {
			strcpy(entry->ip, ip);
			entry->port = port;

			if (NULL != bdi)
				memcpy(&entry->bdi, bdi, sizeof(entry->bdi));

			list_add(&entry->node, &ctl->lsthd);
			ctl->nmbdi_cnt++;

			ret = 0;
		}
	}
	mutex_unlock(ctl->lock);

	return ret;
}

int nms_glb_get_bdinfo(void *hd, char *ip, int port, struct nmboard_info *bdi)
{
	struct nms_bdinfo *entry = NULL;
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;

	if (NULL != hd && NULL != ip && NULL != bdi)
	{
		mutex_lock(ctl->lock);
		entry = nms_glb_find_entry_byip(hd, ip, port);
		if (NULL != entry) {
			memcpy(bdi, &entry->bdi, sizeof(entry->bdi));
		}
		mutex_unlock(ctl->lock);

		return 0;
	}

	return -1;
}

int nms_glb_get_ip_port(void *hd, char *ip, int *port, struct nm_lkaddr_info *lki)
{
	struct nms_bdinfo *entry = NULL;
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;

	if (NULL != hd && NULL != ip && NULL != port && NULL != lki)
	{
		mutex_lock(ctl->lock);
		entry = nms_glb_find_entry_bylki(ctl, lki);
		if (NULL != entry) {
			strcpy(ip, entry->ip);
			*port = entry->port;
		}
		mutex_unlock(ctl->lock);

		return 0;
	}

	return -1;
}

int nms_glb_get_all_bdinfo(void *hd, 
		int (*func)(char *ip, int port, struct nmboard_info bdi, void *arg), void *arg)
{
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;
	struct list_head *p = NULL;
	struct nms_bdinfo *entry = NULL;

	if (!ctl)
		return -1;

	mutex_lock(ctl->lock);
	list_for_each(p, &ctl->lsthd) {
		entry = list_entry(p, struct nms_bdinfo, node);
		if (func) {
			func(entry->ip, entry->port, entry->bdi, arg);
		}
	}
	mutex_unlock(ctl->lock);

	return 0;
}

void *nms_glb_get_all_bdinfo_pkt(void *hd, int *dlen)
{
	int length = 0, len = 0;
	unsigned char *oph = NULL;
	unsigned char *bdi = NULL;
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;
	struct list_head *p = NULL;
	struct nms_bdinfo *entry = NULL;

	if (!hd || !dlen)
		return NULL;

	mutex_lock(ctl->lock);
	*dlen = ctl->nmbdi_cnt * sizeof(struct nmboard_info) + 1;
	len = PKT_LEN + NMPKT_LEN + *dlen;
	oph = malloc(len);
	if (NULL != oph) {
		*(oph + PKT_LEN + NMPKT_LEN) = ctl->nmbdi_cnt;
		bdi = (unsigned char *)(oph + PKT_LEN + NMPKT_LEN + 1);
		list_for_each(p, &ctl->lsthd) {
			entry = list_entry(p, struct nms_bdinfo, node);
			if (NULL != entry && bdi < (oph + len)) {
				length = nm_glb_code_bdinfo(bdi, entry->bdi);
				if (length > 0) 
					bdi += length;
			}
		}
	}
	mutex_unlock(ctl->lock);

	return oph;
}

int nms_glb_del_bdinfo(void *hd, char *ip, int port)
{
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;
	struct list_head *p, *n = NULL;
	struct nms_bdinfo *entry = NULL;

	if (!hd || !ip)
		return -1;

	mutex_lock(ctl->lock);
	list_for_each_safe(p, n, &ctl->lsthd) {
		entry = list_entry(p, struct nms_bdinfo, node);
		if (!strcmp(ip, entry->ip) && port == entry->port) {

			list_del(p);
			ctl->nmbdi_cnt--;

			free(entry);
			mutex_unlock(ctl->lock);

			return 0;
		}
	}

	mutex_unlock(ctl->lock);
	return -1;
}

int nms_glb_release_bdinfo(void *hd)
{
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;
	struct list_head *p, *n = NULL;
	struct nms_bdinfo *entry = NULL;

	if (!ctl)
		return -1;

	mutex_lock(ctl->lock);
	list_for_each_safe(p, n, &ctl->lsthd) {
		entry = list_entry(p, struct nms_bdinfo, node);
		if (entry) {
			list_del(p);
			free(entry);
		}
	}
	ctl->nmbdi_cnt = 0;

	mutex_unlock(ctl->lock);
	return 0;
}

int nms_glb_close(void *hd)
{
	struct nms_glb_ctl *ctl = (struct nms_glb_ctl *)hd;

	if (!ctl)
		return -1;

	nms_glb_release_bdinfo(ctl);
	if (NULL != ctl->lock)
		mutex_close(ctl->lock);

	free(ctl);
	ctl = NULL;

	return 0;
}

int nm_glb_code_bdinfo(unsigned char *data, struct nmboard_info bdi)
{
	unsigned char *p = data;

	if (!data)
		return -1;
	
	CODING_8(data, bdi.rack);
	CODING_8(data, bdi.shelf);
	CODING_8(data, bdi.slot);
	CODING_8(data, bdi.subslot);
	CODING_32(data, bdi.comp); /* component */
	CODING_8(data, bdi.bdtype);
	CODING_8(data, bdi.stat);

	return data - p;
}

int nm_glb_decode_bdinfo(void *data, struct nmboard_info *bdi)
{
	int len = 0;

	if (!data || !bdi)
		return -1;

	len = sizeof(*bdi);
	DECODE_8(data, bdi->rack, len);
	DECODE_8(data, bdi->shelf, len);
	DECODE_8(data, bdi->slot, len);
	DECODE_8(data, bdi->subslot, len);
	DECODE_32(data, bdi->comp, len);
	DECODE_8(data, bdi->bdtype, len);
	DECODE_8(data, bdi->stat, len);
	
	return 0;
}

int nm_glb_decode_lkaddr_info(void *data, struct nm_lkaddr_info *lki)
{
	int len = 0;

	if (!data || !lki)
		return -1;

	len = sizeof(*lki);
	DECODE_8(data, lki->rack, len);
	DECODE_8(data, lki->shelf, len);
	DECODE_8(data, lki->slot, len);
	DECODE_8(data, lki->subslot, len);
	
	return 0;
}
