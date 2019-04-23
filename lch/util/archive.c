/*
 *
 * archive.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "os.h"
#include "aplog.h"
#include "archive.h"

#define MAX_CONCURRENT_OPENED_FILES 32
#define TIMET_HISTORY_NUM           128

#ifdef WIN32
#pragma warning(disable : 4996 ; disable : 4267 )
#define snprintf _snprintf
#endif

struct archive_handle {
	char *path;     /* The directory to archive the data         */
	char *prefix;   /* To construct the filename                 */
	char *suffix;   /* To construct the filename                 */
	char *fmtmin;   /* Datetime format: year, month, day, hour and minute */
	char *fmtsec;   /* Datetime format: second                            */
	int   quota;    /* The maximum percentage of FS we could use */
	int   period;
	int   concurrent;  /* Max concurrent openned files allowed   */
	int   direct;   /* Whether write file direct                 */

	/* Callback to really print out the data */
	int (*format_output)(void *fp, void *buf, int len);

	/* Private data */
	int   oldest;
	FILE      *fp[MAX_CONCURRENT_OPENED_FILES];
	unsigned int timet[MAX_CONCURRENT_OPENED_FILES];

	unsigned int th[TIMET_HISTORY_NUM];
	int          thptr;

	unsigned long long cnt;
};

static __inline int archive_matched_timet(unsigned int timeta, int period,
		unsigned int timetb)
{
	return ((timetb >= timeta) && (timetb < (timeta + period)));
}

static __inline int archive_matched_openned_file(struct archive_handle *h,
		int i, unsigned int timet)
{
	return archive_matched_timet(h->timet[i], h->period, timet);
}

static __inline void archive_form_filename(struct archive_handle *h,
		char *partname, int size, unsigned int timet)
{
	time_t    t = (time_t)timet;
	struct tm now;
	char      format[256];


#ifndef WIN32
	localtime_r((const time_t *)&t, &now);
#else
	_localtime64_s(&now,(const time_t *)&t);
#endif
	sprintf(format, "%%s%%c%%s%s%s%%s.part", h->fmtmin, h->fmtsec ? h->fmtsec: "");

	memset(partname, 0, size);
	if (h->fmtsec) {
		snprintf(partname, size - 1, format,
				h->path, FILENAME_SEPERATOR, h->prefix,
				now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
				now.tm_hour, now.tm_min, now.tm_sec, h->suffix);
	}
	else {
		snprintf(partname, size - 1, format,
				h->path, FILENAME_SEPERATOR, h->prefix,
				now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
				now.tm_hour, now.tm_min, h->suffix);
	}
}

static __inline int archive_open_new_file(struct archive_handle *h,
		int i, unsigned int timet)
{
	char partname[MAX_PATH];
	char realname[MAX_PATH];
	char name[MAX_PATH];
	int  len;

	archive_form_filename(h, partname, MAX_PATH, timet);

	strcpy(realname, partname);
	len = strlen(realname);
	realname[len - 5] = 0;

	if (h->direct) {
		strcpy(name, realname);
	} else {
		rename(realname, partname);
		strcpy(name, partname);
	}

	h->fp[i] = fopen(name, "ab");
	if (h->fp[i]) {
		LOGINFO2("Archive: file %d: %s: openned.", i, name);
		h->timet[i] = timet;
		return 0;
	}
	LOGERROR2("Archive: file %d: %s: openning failed.", i, name);

	return -1;
}

static int archive_prepare_file(struct archive_handle *h, unsigned int timet)
{
	int i, j;

	/* Check if 'timet' matches one of the openned files.
	 *
	 * TODO: Should be better to begin searching from the most recently
	 *       openned file.
	 */
	for (i = 0; i < h->concurrent; i++) {
		if (h->fp[i] && archive_matched_openned_file(h, i, timet))
			return i;
	}

	/* Reached concurrent limit, try reusing the oldest entry. */
	i = h->oldest;
	if (h->fp[i]) {
		char partname[MAX_PATH];
		char realname[MAX_PATH];
		int  len;

		archive_form_filename(h, partname, MAX_PATH, h->timet[i]);

		strcpy(realname, partname);
		len = strlen(realname);
		realname[len - 5] = 0;

		if (h->direct) {
			LOGINFO2("Archive: file %d: %s: closed.", i, realname);
		} else {
			LOGINFO2("Archive: file %d: %s: closed.", i, partname);
		}
		fclose(h->fp[i]);
		h->fp[i] = NULL;

		if (!h->direct) {
			rename(partname, realname);
			LOGINFO2("Archive: file %s renamed to %s.",
					partname, realname);
		}
	}

	/* Check if 'timet' belongs to an file we've already opened but
	 * closed. If it does, reuse that file.
	 */
	j = h->thptr;
	while (1) {
		if (archive_matched_timet(h->th[j], h->period, timet)) {
			timet = h->th[j];
			break;
		}

		--j;
		if (j == -1)
			j = TIMET_HISTORY_NUM - 1;
		if (j == h->thptr) {
			h->thptr = h->thptr + 1;
			if (h->thptr == TIMET_HISTORY_NUM)
				h->thptr = 0;
			h->th[h->thptr] = timet;
			j = h->thptr;
			break;
		}
	}

	/* Do open it */
	if (archive_open_new_file(h, i, timet) < 0) {
		return -1;
	}

	/* Locate the oldest entry */
	timet = (unsigned int)-1;
	for (j = 0; j < h->concurrent; j++) {
		if (!h->fp[j]) {
			h->oldest = j;
			return i;
		}

		if (h->timet[j] < timet) {
			timet = h->timet[j];
			h->oldest = j;
		}
	}

	return i;
}

void *archive_open(char *path, char *prefix, char *suffix,
		int quota, int period, int concurrent,
		int (*format_output)(void *fp, void *buf, int len))
{
	struct archive_handle *h;
	int i;

	if (!path || !prefix || !suffix || (quota < 0) || (quota > 99) ||
	    (concurrent < 1) || (concurrent > MAX_CONCURRENT_OPENED_FILES) ||
	    !format_output) {
		LOGERROR("archive_open: invalid parameter(s)");
		return NULL;
	}

	if (period < 1) {
		LOGERROR1("archive_open: invalid period: %d.", period);
		return NULL;
	}

	h = malloc(sizeof(struct archive_handle));
	if (h == NULL)
		return NULL;

	h->path          = strdup(path);
	h->prefix        = strdup(prefix);
	h->suffix        = strdup(suffix);
	h->fmtmin        = strdup("_%04d_%02d_%02d_%02d_%02d");
	h->fmtsec        = NULL;
	h->quota         = quota;
	h->period        = period * 60;
	h->format_output = format_output;
	h->concurrent    = concurrent;
	h->direct        = 0; /* default write part file firstly */

	h->oldest = 0;
	for (i = 0; i < MAX_CONCURRENT_OPENED_FILES; i++) {
		h->fp[i] = NULL;
		h->timet[i] = 0;
	}

	h->thptr = 0;
	for (i = 0; i < TIMET_HISTORY_NUM; i++) {
		h->th[i] = 0;
	}

	h->cnt = 0;

	return h;
}

int archive_set_write_direct(void *hd, int direct)
{
	struct archive_handle *h = (struct archive_handle *)hd;

	if (!h)
		return -1;

	h->direct = direct;

	return 0;
}

int archive_set_datetime_fmt(void *hd, char *fmt_min, char *fmt_sec)
{
	struct archive_handle *h = (struct archive_handle *)hd;
	char *p;

	if (!h)
		return -1;

	if (fmt_min) {
		p = strdup(fmt_min);
		if (!p)
			return -1;

		if (h->fmtmin)
			free(h->fmtmin);
		h->fmtmin = p;
	}

	if (fmt_sec && *fmt_sec) {
		p = strdup(fmt_sec);
		if (!p)
			return -1;

		if (h->fmtsec)
			free(h->fmtsec);
		h->fmtsec = p;
	}

	return 0;
}

int archive_write(void *hd, void *buf, int len, unsigned int timet)
{
	struct archive_handle *h = (struct archive_handle *)hd;
	int i;

	if (!h)
		return -1;

	timet = timet - (timet % h->period);

	if ((i = archive_prepare_file(h, timet)) < 0) {
		LOGERROR("Cannot determine which file to write data to.");
		return -1;
	}

	h->cnt++;

	return h->format_output(h->fp[i], buf, len);
}

void archive_close(void *hd)
{
	char partname[MAX_PATH];
	char realname[MAX_PATH];
	struct archive_handle *h = (struct archive_handle *)hd;
	int i;
	int  len;

	if (!h)
		return;

	for (i = 0; i < MAX_CONCURRENT_OPENED_FILES; i++) {
		if (!(h->fp[i]))
			continue;

		archive_form_filename(h, partname, MAX_PATH, h->timet[i]);

		strcpy(realname, partname);
		len = strlen(realname);
		realname[len - 5] = 0;

		if (h->direct) {
			LOGINFO2("Archive: file %d: %s: closed.", i, partname);
		} else {
			LOGINFO2("Archive: file %d: %s: closed.", i, realname);
		}
		fclose(h->fp[i]);

		if (!h->direct) {
			rename(partname, realname);
			LOGINFO2("Archive: file %s renamed to %s.",
					partname, realname);
		}
	}

	LOG3("archive: prefix %s, suffix %s, %llu items saved.",
			h->prefix, h->suffix, h->cnt);

	if (h->path)
		free(h->path);
	if (h->prefix)
		free(h->prefix);
	if (h->suffix)
		free(h->suffix);
	if (h->fmtmin)
		free(h->fmtmin);
	if (h->fmtsec)
		free(h->fmtsec);

	free(h);
}

