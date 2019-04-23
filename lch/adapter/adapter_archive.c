/*
 * (C) Copyright 2008
 * Hu Chunlin <chunlin.hu@gmail.com>
 *
 * adapter_archive.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "os.h"
#include "aplog.h"
#include "cconfig.h"
#include "pkt.h"
#include "archive.h"
#include "adapter.h"
#include "adapter_archive.h"

#define ADAPTER_ARCHIVE_NAME "Adapter.Archive"

struct adapter_archive_cfg {
	char path[256];
	char prefix[64];
	char datefmt[64];
	char secfmt[16];
	char suffix[16];
	int  quota;
	int  period;
	int  concurrent;
	int  disabled;
};

struct adapter_archive_ctl {
	void *ar;

	struct adapter_archive_cfg cfg;
};

static inline int load_ar_option(unsigned long h,
		char *sname, char *option, int size, char *dest)
{
	char value[1024];

	if (CfgGetValue(h, sname, option, value, 1, 1) == -1) {
		LOGERROR("No %s defined in section [%s].", option, sname);
		return -1;
	}
	if (strlen(value) >= (size_t)size) {
		LOGERROR("Invalid %s (%s) defined in section [%s].",
				option, value, sname);
		return -1;
	}
	strcpy(dest, value);
	LOGINFO("[%s] %s = %s", sname, option, value);

	return 0;
}

static int load_cfgfile_archive(unsigned long h, char *sname,
		struct adapter_archive_cfg *cfg)
{
	char value[1024];

	if (load_ar_option(h, sname, (char *)"path", 256, cfg->path) < 0)
		return -1;

	if (load_ar_option(h, sname, (char *)"prefix", 64, cfg->prefix) < 0)
		return -1;

	if (load_ar_option(h, sname, (char *)"datefmt", 64, cfg->datefmt) < 0)
		return -1;

	load_ar_option(h, sname, (char *)"secfmt", 16, cfg->secfmt);

	if (load_ar_option(h, sname, (char *)"suffix", 16, cfg->suffix) < 0)
		return -1;

	/* quota */
	if (CfgGetValue(h, sname, "quota", value, 1, 1) == -1) {
		LOGINFO("No quota defined in section [%s], using default.",
				sname);
	}
	else {
		cfg->quota = atoi(value);
		if ((cfg->quota < 0) || (cfg->quota > 99)) {
			LOGERROR("Invalid quota %s defined in section [%s].",
					value, sname);
			return -1;
		}
	}
	LOGINFO("[%s] quota = %s", sname, value);

	/* period */
	if (CfgGetValue(h, sname, "period", value, 1, 1) == -1) {
		LOGERROR("No period defined in section [%s].", sname);
		return -1;
	}
	else {
		cfg->period = atoi(value);
		if (cfg->period < 1) {
			LOGERROR("Invalid period %s defined in section [%s].",
					value, sname);
			return -1;
		}
	}
	LOGINFO("[%s] period = %s", sname, value);

	/* concurrent */
	if (CfgGetValue(h, sname, "concurrent", value, 1, 1) == -1) {
		LOGINFO("No concurrent defined in section [%s], using default.",
				sname);
		cfg->concurrent = 3;
	}
	else {
		cfg->concurrent = atoi(value);
		if ((cfg->concurrent < 1) || (cfg->concurrent > 32)) {
			LOGERROR("Invalid concurrent %s defined in section [%s].",
					value, sname);
			return -1;
		}
	}
	LOGINFO("[%s] concurrent = %d", sname, cfg->concurrent);

	/* disabled */
	if (CfgGetValue(h, sname, "disabled", value, 1, 1) != -1) {
		if (strcmp(value, "yes") == 0)
			cfg->disabled = 1;
	}
	LOGINFO("[%s] disabled = %s", sname, cfg->disabled ? "yes" : "no");

	return 0;
}

static void adapter_archive_exit(struct adapter_archive_ctl *ctl)
{
	if (ctl) {
		free(ctl);
	}
}

static int adapter_archive_default_dumper(void *fp, void *buf, int len)
{
	return fwrite(buf, len, 1, (FILE *)fp);
}

static struct adapter_archive_ctl *adapter_archive_init(void *adap,
		unsigned long cfghd, char *sname,
		int (*format_output)(void *fp, void *buf, int len))
{
	struct adapter_archive_ctl *ctl;
	struct adapter_archive_cfg *cfg;
	int (*dumper)(void *fp, void *buf, int len);

	ctl = (struct adapter_archive_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL)
		return NULL;
	memset(ctl, 0, sizeof(struct adapter_archive_ctl));
	cfg = &(ctl->cfg);

	if (load_cfgfile_archive(cfghd, sname, cfg) < 0) {
		LOGERROR("%s: Loading configuration from section [%s] failed.",
				adapter_get_name(adap), sname ? : "NULL");
		goto adapter_archive_init_failed;
	}

	if (cfg->disabled)
		return ctl;
	
	dumper = format_output ? format_output : adapter_archive_default_dumper;
	ctl->ar = archive_open(cfg->path, cfg->prefix, cfg->suffix,
			cfg->quota, cfg->period, cfg->concurrent, dumper);
	if (ctl->ar == NULL) {
		LOGERROR("%s: Creating archive failed.", adapter_get_name(adap));
		goto adapter_archive_init_failed;
	}
	if (archive_set_datetime_fmt(ctl->ar, cfg->datefmt, cfg->secfmt) < 0) {
		LOGERROR("%s: Setting datetime format failed.", adapter_get_name(adap));
		goto adapter_archive_init_failed;
	}

	return ctl;

adapter_archive_init_failed:
	free(ctl);
	return NULL;
}

int adapter_archive_write(void *adap, void *data, int len)
{
	struct adapter_archive_ctl *ctl;

	ctl = (struct adapter_archive_ctl *)adapter_get_data(adap);
	if (!ctl || !ctl->ar)
		return -1;

	return archive_write(ctl->ar, data, len, pkthdr_get_ts_s((pkt_hdr *)data));
}

static void adapter_archive_close(void *adap)
{
	struct adapter_archive_ctl *ctl;

	ctl = (struct adapter_archive_ctl *)adapter_get_data(adap);
	if (ctl) {
		if (ctl->ar)
			archive_close(ctl->ar);
	}

	adapter_archive_exit(ctl);
}

void *adapter_register_archive(unsigned long cfghd, char *section,
		int (*format_output)(void *fp, void *buf, int len))
{
	void *adap;
	struct adapter_archive_ctl *ctl;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;
	adapter_set_name(adap, ADAPTER_ARCHIVE_NAME);

	ctl = adapter_archive_init(adap, cfghd, section ? : ADAPTER_ARCHIVE_NAME,
			format_output);
	if (ctl == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_data(adap, ctl);

	adapter_set_write(adap, adapter_archive_write);
	adapter_set_close(adap, adapter_archive_close);

	return adap;
}
