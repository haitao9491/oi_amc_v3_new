/*
 *
 * adapter_file.c - A brief description goes here.
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
#include "pkt.h"
#include "misc.h"
#include "adapter.h"
#include "adapter_file.h"

#define ADAPTER_FILE_NAME "Adapter.FILE"

struct adapter_file_ctl {
	int num;
	char **files;

	int speed;
	int loop;
	int wait;

	/* File tracking */
	void *pfp;
	int   currfile;

	/* Pace control */
	unsigned int ps, pns;
	unsigned int dosleep;

	/* Statistics */
	unsigned long long totalpkts;
	unsigned long long totalbytes;
};

static void adapter_file_exit(struct adapter_file_ctl *lst)
{
	if (lst) {
		if (lst->files) {
			int i;

			for (i = 0; i < lst->num; i++) {
				if (*(lst->files + i))
					free(*(lst->files + i));
			}
			free(lst->files);
		}

		free(lst);
	}
}

static struct adapter_file_ctl *adapter_file_init(void *adap,
		unsigned long cfghd, char *sname)
{
	int i;
	char value[1024];
	struct adapter_file_ctl *lst;

	lst = (struct adapter_file_ctl *)malloc(sizeof(*lst));
	if (lst == NULL)
		return NULL;
	memset(lst, 0, sizeof(struct adapter_file_ctl));
	lst->currfile = -1;

	i = CfgGetCount(cfghd, sname, "datafile", 1);
	if (i < 1) {
		LOGERROR2("%s: Section %s: No datafile defined.",
				adapter_get_name(adap), sname);
		goto adapter_file_init_filenamemalloced;
	}

	lst->num = i;
	lst->files = (char **)malloc(sizeof(char *) * lst->num);
	if (lst->files == NULL) {
		LOGERROR2("%s: Section %s: Insufficient memory for file paths.",
				adapter_get_name(adap), sname);
		goto adapter_file_init_filenamemalloced;
	}
	for (i = 0; i < lst->num; i++)
		*(lst->files + i) = NULL;

	for (i = 0; i < lst->num; i++) {
		if (CfgGetValue(cfghd, sname, "datafile", value, i+1, 1) == -1){
			LOGERROR3("%s: Section %s: Loading datafile %d failed.",
					adapter_get_name(adap), sname, i + 1);
			goto adapter_file_init_filenamemalloced;
		}
		*(lst->files + i) = strdup(value);
		if (*(lst->files + i) == NULL) {
			LOGERROR4("%s: Section %s: Loading datafile %d (%s): "
					"Insufficient memory.",
					adapter_get_name(adap),
					sname, i + 1, value);
			goto adapter_file_init_filenamemalloced;
		}
		LOGINFO4("%s: Section %s: datafile %d (%s) loaded.",
				adapter_get_name(adap), sname,
				i + 1, *(lst->files + i));
	}

	if (CfgGetValue(cfghd, sname, "speed", value, 1, 1) == -1) {
		lst->speed = 1;
		LOGINFO2("%s: Section %s: Loading speed failed, using default [1].", adapter_get_name(adap), sname);
	}
	else {
		lst->speed = atoi(value);
		if (lst->speed < 1) {
			LOGERROR2("%s: Loading speed failed: invalid value %s.",
					adapter_get_name(adap), value);
			goto adapter_file_init_filenamemalloced;
		}
	}
	LOGINFO3("%s: Section %s: Speed: %d",
			adapter_get_name(adap), sname, lst->speed);

	if (CfgGetValue(cfghd, sname, "loop", value, 1, 1) == -1) {
		lst->loop = 1;
		LOGINFO2("%s: Section %s: Loading loop failed, using default [1].", adapter_get_name(adap), sname);
	}
	else {
		lst->loop = atoi(value);
	}
	LOGINFO3("%s: Section %s: Loop: %d",
			adapter_get_name(adap), sname, lst->loop);

	if (CfgGetValue(cfghd, sname, "wait", value, 1, 1) == -1) {
		lst->wait = 0;
		LOGINFO("%s: Section %s: Loading wait failed, using default [0].",
				adapter_get_name(adap), sname);
	}
	else {
		lst->wait = atoi(value);
		if (lst->wait < 0)
			lst->wait = 0;
	}
	LOGINFO("%s: Section %s: wait: %d",
			adapter_get_name(adap), sname, lst->wait);

	return lst;

adapter_file_init_filenamemalloced:
	adapter_file_exit(lst);
	return NULL;
}

static void adapter_file_try_delay(struct adapter_file_ctl *lst,
		unsigned int s, unsigned int ns)
{
	int diff;

	if ((lst->speed < 1) || (lst->speed >= 100))
		return;

	if (lst->ps != 0) {
		diff = time_diff_us(lst->ps, lst->pns, s, ns) / lst->speed;
		if (diff <= 0)
			return;

		lst->dosleep += diff;
		if (lst->dosleep >= 1000000)
			lst->dosleep = 999999;
		if (lst->dosleep >= 10000) {
			SLEEP_US((lst->dosleep / 10000) * 10000);
			lst->dosleep = 0;
		}
	}
	lst->ps = s;
	lst->pns = ns;
}

static void *adapter_file_read(void *adap)
{
	pkt_hdr *ph;
	unsigned int s, ns;
	struct adapter_file_ctl *lst;

	lst = (struct adapter_file_ctl *)adapter_get_data(adap);
	if (!lst)
		return NULL;

	while (lst->wait > 0) {
		SLEEP_S(1);
		lst->wait--;
	}

adapter_file_read_try:
	ph = lst->pfp ? (pkt_hdr *)pkt_read_file(lst->pfp) : NULL;
	if (ph == NULL) {
		/* Step to next data file */
		lst->currfile++;

		if (lst->currfile >= lst->num) {
			if (!lst->loop) {
				LOGINFO2("%s: Finished processing all %d files.", adapter_get_name(adap), lst->num);
				return NULL;
			}
			lst->currfile = 0;
		}

		/* Open next data file */
		if (lst->pfp)
			pkt_close_file(lst->pfp);
		lst->pfp = pkt_open_file(*(lst->files + lst->currfile), "rb",9);
		lst->ps = 0;
		lst->pns = 0;
		LOGINFO2("%s: Switching to: %s.", adapter_get_name(adap),
				*(lst->files + lst->currfile));

		goto adapter_file_read_try;
	}

	/* Got a packet, determine how long we should wait. */
	pkthdr_get_ts(ph, &s, &ns);
	adapter_file_try_delay(lst, s, ns);

	lst->totalpkts++;
	lst->totalbytes += pkthdr_get_plen(ph);

	return ph;
}

static void adapter_file_close(void *adap)
{
	struct adapter_file_ctl *lst;

	lst = (struct adapter_file_ctl *)adapter_get_data(adap);
	if (lst) {
		if (lst->pfp)
			pkt_close_file(lst->pfp);

		LOGINFO3("%s: Total %llu packets (%llu bytes) read.",
				adapter_get_name(adap),
				lst->totalpkts, lst->totalbytes);
	}

	adapter_file_exit(lst);
}

static void *_adapter_register_file(unsigned long cfghd, char *section)
{
	void *adap;
	struct adapter_file_ctl *lst;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;
	adapter_set_name(adap, ADAPTER_FILE_NAME);

	lst = adapter_file_init(adap, cfghd, section ? section : ADAPTER_FILE_NAME);
	if (lst == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_data(adap, lst);

	adapter_set_read(adap, adapter_file_read);
	adapter_set_close(adap, adapter_file_close);

	return adap;
}

void *adapter_register_file(unsigned long cfghd, char *section)
{
	if (cfghd == 0ul)
		return NULL;

	return _adapter_register_file(cfghd, section);
}

void *adapter_register_file_cfgfile(char *cfgfile, char *section)
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitialize(cfgfile);
	if (cfghd != 0ul) {
		p = _adapter_register_file(cfghd, section);
		CfgInvalidate(cfghd);
	}

	return p;
}

void *adapter_register_file_cfgstr(char *cfgstr, char *section)
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitializeString(cfgstr);
	if (cfghd != 0ul) {
		p = _adapter_register_file(cfghd, section);
		CfgInvalidate(cfghd);
	}

	return p;
}

