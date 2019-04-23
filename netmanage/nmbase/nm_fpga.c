/*
 *
 * nm_fpga.c - A brief description goes here.
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
#include "nm_fpga.h"

struct fpga_component {
	int id;
	int type;
	struct fpga_status st;
	struct fpga_stat   sta;
	struct fpga_fstat  fsta;
	struct list_head node;
};

struct fpga_component_ctl {
	struct list_head lsthd;
	int cnt;
};

void *nm_fpga_open(void)
{
	struct fpga_component_ctl *ctl = NULL;

	ctl = (struct fpga_component_ctl *)malloc(sizeof(*ctl));
	if (!ctl)
		return NULL;

	memset(ctl, 0, sizeof(*ctl));
	INIT_LIST_HEAD(&ctl->lsthd);
	ctl->cnt = 0;

	return ctl;
}

int nm_fpga_add_id_type(void *hd, int id, int type)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;
	struct list_head *p = NULL;
	struct fpga_component *entry = NULL;

	if (!ctl)
		return -1;

	list_for_each(p, &ctl->lsthd) {
		entry = list_entry(p, struct fpga_component, node);
		if (entry->id == id && entry->type == type) {
			return 0;
		}
	}

	entry = (struct fpga_component *)malloc(sizeof(*entry));
	if (!entry)
		return -1;

	memset(entry, 0, sizeof(*entry));
	entry->id    = id;
	entry->type  = type;
	list_add(&entry->node, &ctl->lsthd);
	ctl->cnt++;

	return 0;
}

int nm_fpga_add_data(void *hd, 
		int id, int type, void *data, int flag)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;
	struct list_head *p = NULL;
	struct fpga_component *entry = NULL;
	unsigned char *da = (unsigned char *)data;
	int pcnt = 0;
	int len = 0;

	if (!ctl || !data)
		return -1;

	list_for_each(p, &ctl->lsthd) {
		entry = list_entry(p, struct fpga_component, node);
		if (entry->id == id && entry->type == type) {
			pcnt = *da++;
			if (flag == FPGA_DATA_STATUS) {
				len = pcnt * sizeof(struct fpga_port_status);
				if (entry->st.pcnt == pcnt && entry->st.st != NULL) {
					memcpy(entry->st.st, da, len);
				}
				else {
					if (entry->st.st) {
						free(entry->st.st);
						entry->st.st = NULL;
					}
					entry->st.pcnt = pcnt;
					entry->st.st = malloc(len);
					if (entry->st.st) {
						memcpy(entry->st.st, da, len);
					}
				}
				return 0;
			}
			else if (flag == FPGA_DATA_STAT) {
				len = pcnt * sizeof(struct fpga_port_stat);
				if (entry->sta.pcnt == pcnt && entry->sta.sta != NULL) {
					memcpy(entry->sta.sta, da, len);
				}
				else {
					if (entry->sta.sta) {
						free(entry->sta.sta);
						entry->sta.sta = NULL;
					}
					entry->sta.pcnt = pcnt;
					entry->sta.sta = malloc(len);
					if (entry->sta.sta) {
						memcpy(entry->sta.sta, da, len);
					}
				}
				return 0;
			}
			else if (flag == FPGA_DATA_FSTAT) {
				len = pcnt * sizeof(struct fpga_port_fstat);
				if (entry->fsta.pcnt == pcnt && entry->fsta.fsta != NULL) {
					memcpy(entry->fsta.fsta, da, len);
				}
				else {
					if (entry->fsta.fsta) {
						free(entry->fsta.fsta);
						entry->fsta.fsta = NULL;
					}
					entry->fsta.pcnt = pcnt;
					entry->fsta.fsta = malloc(len);
					if (entry->fsta.fsta) {
						memcpy(entry->fsta.fsta, da, len);
					}
				}
				return 0;
			}
			else {
				LOGERROR("%s: add data failed flag %d unknown", __func__, flag);
				return -1;
			}
		}
	}

	return -1;
}

int nm_fpga_get_id_type(void *hd, 
		int (*fpga_func)(int id, int type, void *arg), void *arg)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;
	struct list_head *p = NULL;
	struct fpga_component *entry = NULL;

	if (!hd)
		return -1;

	list_for_each(p, &ctl->lsthd) {
		entry = list_entry(p, struct fpga_component, node);
		if (entry != NULL && fpga_func) {
			fpga_func(entry->id, entry->type, arg);
		}
	}

	return 0;
}

int nm_fpga_get_data(void *hd, 
		int id, int type, void *da, int flag)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;
	struct list_head *p = NULL;
	struct fpga_component *entry = NULL;

	if (!ctl || !da)
		return -1;

	list_for_each(p, &ctl->lsthd) {
		entry = list_entry(p, struct fpga_component, node);
		if (entry->id == id && entry->type == type) {
			if (flag == FPGA_DATA_STATUS) {
				struct fpga_status *st = (struct fpga_status *)da;
				st->pcnt = entry->st.pcnt; 
				st->st   = entry->st.st; 
			}
			else if (flag == FPGA_DATA_STAT) {
				struct fpga_stat *sta = (struct fpga_stat *)da;
				sta->pcnt = entry->sta.pcnt; 
				sta->sta  = entry->sta.sta; 
			}
			else if (flag == FPGA_DATA_FSTAT) {
				struct fpga_fstat *fsta = (struct fpga_fstat *)da;
				fsta->pcnt = entry->fsta.pcnt; 
				fsta->fsta = entry->fsta.fsta; 
			}
			else {
				LOGERROR("%s: input flag %d unknown", __func__, flag);
				return -1;
			}
			return 0;
		}
	}

	return -1;
}

void *nm_fpga_get_id_type_pkt(void *hd, struct nm_lkaddr_info *lki, int *dlen)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;
	struct list_head *p = NULL;
	struct fpga_component *entry = NULL;
	unsigned char *op, *oph = NULL;
	int len = 0;

	if (!hd)
		return NULL;

	*dlen = sizeof(*lki) + 1 + ctl->cnt * 2; /* id+type=2Byte */
	len = PKT_LEN + NMPKT_LEN + *dlen;
	oph = malloc(len);
	if (!oph) {
		LOGERROR("%s: malloc failed!", __func__);
		return NULL;
	}

	memset(oph, 0, len);
	op = oph + PKT_LEN + NMPKT_LEN;
	memcpy(op, lki, sizeof(*lki));
	op += sizeof(*lki);
	*op++ = ctl->cnt;

	if (ctl->cnt > 0) {
		list_for_each(p, &ctl->lsthd) {
			entry = list_entry(p, struct fpga_component, node);
			*op++ = entry->id;
			*op++ = entry->type;
		}
	}

	return oph;
}

void *nm_fpga_get_data_pkt(void *hd, 
		int id, int type, struct nm_lkaddr_info *lki, int *dlen, int flag)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;
	struct list_head *p = NULL;
	struct fpga_component *entry = NULL;
	unsigned char *op, *oph = NULL;
	int len = 0;

	if (!ctl || !lki || !dlen)
		return NULL;

	list_for_each(p, &ctl->lsthd) {
		entry = list_entry(p, struct fpga_component, node);
		if (entry->id == id && entry->type == type) {
			if (flag == FPGA_DATA_STATUS) {
				*dlen = sizeof(*lki) + 1 + 1 + 1 + entry->st.pcnt * sizeof(*(entry->st.st));
			}
			else if (flag == FPGA_DATA_STAT) {
				*dlen = sizeof(*lki) + 1 + 1 + 1 + entry->sta.pcnt * sizeof(*(entry->sta.sta));
			}
			else if (flag == FPGA_DATA_FSTAT) {
				*dlen = sizeof(*lki) + 1 + 1 + 1 + entry->fsta.pcnt * sizeof(*(entry->fsta.fsta));
			}
			else {
				LOGERROR("%s: flag input err %d", __func__, flag);
				return NULL;
			}
			len = PKT_LEN + NMPKT_LEN + *dlen;
			oph = malloc(len);
			if (!oph) {
				LOGERROR("%s: malloc failed", __func__);
				return NULL;
			}

			memset(oph, 0, len);
			op = oph + PKT_LEN + NMPKT_LEN;
			memcpy(op, lki, sizeof(*lki));
			op += sizeof(*lki);
			*op++ = id & 0xff;
			*op++ = type & 0xff;
			if (flag == FPGA_DATA_STATUS) {
				*op++ = entry->st.pcnt;
				memcpy(op, entry->st.st, entry->st.pcnt * sizeof(*(entry->st.st)));
			}
			else if (flag == FPGA_DATA_STAT) {
				*op++ = entry->sta.pcnt;
				memcpy(op, entry->sta.sta, entry->sta.pcnt * sizeof(*(entry->sta.sta)));
			}
			else if (flag == FPGA_DATA_FSTAT) {
				*op++ = entry->fsta.pcnt;
				memcpy(op, entry->fsta.fsta, entry->fsta.pcnt * sizeof(*(entry->fsta.fsta)));
			}

			return oph;
		}
	}

	return NULL;
}

int nm_fpga_del_data(void *hd, int id, int type)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;
	struct list_head *p, *n = NULL;
	struct fpga_component *entry = NULL;

	if (!hd)
		return -1;

	list_for_each_safe(p, n, &ctl->lsthd) {
		entry = list_entry(p, struct fpga_component, node);
		if (entry->id == id && entry->type == type) {
			if (entry->st.st)
				free(entry->st.st);
			if (entry->sta.sta)
				free(entry->sta.sta);
			if (entry->fsta.fsta)
				free(entry->fsta.fsta);

			list_del(p);
			free(entry);
			ctl->cnt--;
			return 0;
		}
	}

	return -1;
}

int nm_fpga_release_data(void *hd)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;
	struct list_head *p, *n = NULL;
	struct fpga_component *entry = NULL;

	if (!hd)
		return -1;

	list_for_each_safe(p, n, &ctl->lsthd) {
		entry = list_entry(p, struct fpga_component, node);
		if (entry->st.st)
			free(entry->st.st);
		if (entry->sta.sta)
			free(entry->sta.sta);
		if (entry->fsta.fsta)
			free(entry->fsta.fsta);

		list_del(p);
		free(entry);
	}

	ctl->cnt = 0;
	return 0;
}

int nm_fpga_close(void *hd)
{
	struct fpga_component_ctl *ctl = (struct fpga_component_ctl *)hd;

	if (!hd)
		return -1;

	nm_fpga_release_data(hd);
	free(ctl);
	ctl = NULL;
	
	return 0;
}

