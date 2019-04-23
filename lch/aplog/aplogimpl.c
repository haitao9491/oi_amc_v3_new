/*
 *
 * aplogimpl.cpp - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#else
#pragma warning(disable : 4996)
#endif

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "os.h"
#include "aplog.h"
#include "mutex.h"

#define LGWR_LOCK(mutex)    { mutex_lock(mutex); }
#define LGWR_UNLOCK(mutex)  { mutex_unlock(mutex); }

#ifdef _USRDLL
DECLARE_LOGHANDLE;
#endif

struct lgwr_hd {
	char  filename[256];
	char  date[64];
	char  buffer[BUFSIZ];

	FILE *fp;

	int   flushnow;
	unsigned long long  size;

	int   level;
	char  title[LGWRLEVELS][16];

	void *mutex;
};

static __inline void lgwr_get_current_time(struct lgwr_hd *lhd)
{
	time_t     timet;
	struct tm  tm;

	timet = time(NULL);
#ifndef WIN32
	if (localtime_r(&timet, &tm)) {
#else
	if (!_localtime64_s(&tm,&timet)) {
#endif
		sprintf(lhd->date, "%04d%02d%02d %02d:%02d:%02d",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
#if defined(OTAP_V1)
				tm.tm_hour + 8, tm.tm_min, tm.tm_sec);
#else
				tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif

	}
	else {
		lhd->date[0] = 0;
	}
}

static void lgwr_inittitle(struct lgwr_hd *lhd)
{
	strcpy(lhd->title[LGWRLEVELEMERGENCY], "EMERGENCY");
	strcpy(lhd->title[LGWRLEVELALERT],     "ALERT");
	strcpy(lhd->title[LGWRLEVELCRITICAL],  "CRITICAL");
	strcpy(lhd->title[LGWRLEVELERROR],     "ERROR");
	strcpy(lhd->title[LGWRLEVELWARN],      "WARN");
	strcpy(lhd->title[LGWRLEVELNOTICE],    "NOTICE");
	strcpy(lhd->title[LGWRLEVELINFO],      "INFO");
	strcpy(lhd->title[LGWRLEVELDEBUG],     "DEBUG");
}

static int lgwr_set_flushnow(struct lgwr_hd *lhd, int flag, int lockme)
{
	int i;

	if(lockme)
		LGWR_LOCK(lhd->mutex);

	if((flag != 1) && (flag != 0)) {
		if(lockme)
			LGWR_UNLOCK(lhd->mutex);
		return(-1);
	}

	i = lhd->flushnow;
	lhd->flushnow = flag;

	if(lhd->flushnow) {
		setbuf(lhd->fp, NULL);
	}
	else {
		setbuf(lhd->fp, lhd->buffer);
	}

	if(lockme)
		LGWR_UNLOCK(lhd->mutex);

	return(i);
}

const void *lgwr_get_title(void *hd, int logLevel)
{
	struct lgwr_hd *lhd;
	char *p;

	if ((lhd = (struct lgwr_hd *)hd) == NULL)
		return "";

	if ((logLevel < 0) && (logLevel >= sizeof(lhd->title)))
		return NULL;
	p = lhd->title[logLevel];
	return p;

}

void *lgwr_open(char *file, int level, int size)
{
	struct lgwr_hd *lhd;

	if ((lhd = (struct lgwr_hd *)malloc(sizeof(*lhd))) == NULL)
		return NULL;

	memset(lhd, 0, sizeof(*lhd));

	if (file && *file)
		lhd->fp = fopen(file, "a");
	if (!lhd->fp)
		lhd->fp = stdout;

	if (lhd->fp && (lhd->fp != stdout)) {
		strcpy(lhd->filename, file);
	}
#ifdef LGWRFLUSHNOW
	lgwr_set_flushnow(lhd, 1, 0);
#else
	lgwr_set_flushnow(lhd, 0, 0);
#endif
	if (size < 1)
		size = 1;
	lhd->size = ((unsigned long long )size) * 1024;
	lhd->level = level;
	lgwr_inittitle(lhd);

	lhd->mutex = mutex_open(NULL);

#ifdef _USRDLL
	__lgwr__handle = lhd;
#endif

	return lhd;
}

void lgwr_close(void *hd)
{
	struct lgwr_hd *lhd;

	if ((lhd = (struct lgwr_hd *)hd) == NULL)
		return;

	if (lhd->fp && (lhd->fp != stdout))
		fclose(lhd->fp);
	if (lhd->mutex)
		mutex_close(lhd->mutex);

	free(hd);
}

void lgwr_set_file(void *hd, char *s, int mode, int lockme)
{
	struct lgwr_hd *lhd;

	if ((lhd = (struct lgwr_hd *)hd) == NULL)
		return;

	if (lockme)
		LGWR_LOCK(lhd->mutex);

	if (lhd->fp && (lhd->fp != stdout))
		fclose(lhd->fp);
	lhd->fp = NULL;

	if (s && *s)
		lhd->fp = fopen(s, (mode ? "w" : "a"));
	if (lhd->fp) {
		if (lhd->filename != s)
			strcpy(lhd->filename, s);
	}
	else {
		memset(lhd->filename, 0, sizeof(lhd->filename));
		lhd->fp = stdout;
	}

	if(lockme)
		LGWR_UNLOCK(lhd->mutex);

	lgwr_set_flushnow(lhd, lhd->flushnow, lockme);
}

void lgwr_set_level(void *hd, int newlevel)
{
	struct lgwr_hd *lhd;

	if ((lhd = (struct lgwr_hd *)hd) == NULL)
		return;

	LGWR_LOCK(lhd->mutex);
	if((newlevel >= 0) && (newlevel < LGWRLEVELS))
		lhd->level = newlevel;
	LGWR_UNLOCK(lhd->mutex);
}

void lgwr_set_size(void *hd, int newsize)
{
	struct lgwr_hd *lhd;

	if ((lhd = (struct lgwr_hd *)hd) == NULL)
		return;

	LGWR_LOCK(lhd->mutex);
	if (newsize > 0)
		lhd->size = ((unsigned long long )newsize) * 1024;
	LGWR_UNLOCK(lhd->mutex);
}

void lgwr_prt(void *hd, const void *dumpdata, int dumplen,
		const char *file, int line, const char *logformat, ...)
{
	struct lgwr_hd *lhd;
#if !defined(OS_WINDOWS)
	int            err;
#endif
	int            offset, i, j, k;
	int            curlevel;
	va_list        ap;
	unsigned char *ucptr;
	const char    *format = logformat;

	if ((lhd = (struct lgwr_hd *)hd) == NULL)
		return;

	if((lhd->fp == NULL) || (format == NULL)) {
		return;
	}

	if((*format == '<') && (*(format + 1) >= '0') && (*(format + 1) <= '7') && (*(format + 2) == '>')) {
		curlevel  = *(format + 1) - '0';
		format   += 3;
	}
	else {
		curlevel  = LGWRLEVELNOLEVEL;
	}
	if(curlevel > lhd->level) {
		return;
	}

#if !defined(OS_WINDOWS)
	err   = errno;	// save the current errno.
#endif
	ucptr = (unsigned char *)dumpdata;

	LGWR_LOCK(lhd->mutex);

	if((lhd->size != 0) && (lhd->fp != stdout) && (FTELL(lhd->fp) >= lhd->size)) {
		char   bakfile[300];

		fclose(lhd->fp);
		lhd->fp = NULL;

		sprintf(bakfile, "%s.bak", lhd->filename);
		unlink(bakfile);
		rename(lhd->filename, bakfile);
		lgwr_set_file(lhd, lhd->filename, 1, 0);
	}
	if(!lhd->fp) {
		LGWR_UNLOCK(lhd->mutex);
		return;
	}

	lgwr_get_current_time(lhd);

#if !defined(OS_WINDOWS)
	fprintf(lhd->fp, "%d %ld %s ", getpid(), syscall(SYS_gettid), lhd->date);
#else
	fprintf(lhd->fp, "%d %d %s ", GetCurrentProcessId(), GetCurrentThreadId(), lhd->date);
#endif

	if (curlevel != LGWRLEVELNOLEVEL)
		fprintf(lhd->fp, "[%s]: ", lhd->title[curlevel]);
	else
		fprintf(lhd->fp, "[VOID]: ");

	va_start(ap, logformat);
	vfprintf(lhd->fp, format, ap);
	va_end(ap);

	if ((curlevel != LGWRLEVELNOLEVEL) && (curlevel <= LGWRLEVELWARN)) {
#if defined(OS_WINDOWS)
		LPVOID lpMsgBuf;
		DWORD  lasterror = GetLastError();
		DWORD  buf_len = FormatMessage(
		                      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
							  FORMAT_MESSAGE_IGNORE_INSERTS,
		                      NULL,
		                      lasterror,
							  0x0409,
		                      (LPTSTR) &lpMsgBuf,
		                      0,
		                      NULL);
		fprintf(lhd->fp, " <<>> FILE[%s], LINE[%d]", file, line);
		if (lasterror) {
			if (buf_len) {
				char *msgbuf = (char *)lpMsgBuf;
				int   iii;

				for (iii = buf_len - 1; iii >= 0; iii--) {
					if ((msgbuf[iii] == '\n') ||
							(msgbuf[iii] == '\r'))
						msgbuf[iii] = 0;
					else
						break;
				}
				fwprintf(lhd->fp, L", ERRNO[%d, %s]",
						lasterror, (LPCSTR)lpMsgBuf);
			}
			else
				fprintf(lhd->fp, ", ERRNO[%d]", lasterror);
		}
		else
			fprintf(lhd->fp, ", ERRNO[0, Application error.]");
		LocalFree(lpMsgBuf);
#else
		fprintf(lhd->fp, " <<>> FILE[%s], LINE[%d]", file, line);
		if(err)
			fprintf(lhd->fp, ", ERRNO[%d, %s]", err, strerror(err));
		else
			fprintf(lhd->fp, ", ERRNO[0, Application error.]");
#endif
	}
	fprintf(lhd->fp, "\n");

	if((dumplen <= 0) || (ucptr == NULL)) {
		LGWR_UNLOCK(lhd->mutex);
		return;
	}

	fprintf(lhd->fp, "OFFSET       -0--1--2--3--4--5--6--7-*-8--9--a--b--c--d--e--f- DISPLAY\n");
	offset = i = 0;
	while(i < dumplen) {
		fprintf(lhd->fp, "%05x(%05d) ", offset, offset);
		for(j = 0; (j < 16) && (i < dumplen); j++, i++) {
			if(j < 7)
				fprintf(lhd->fp, "%02x ", *(ucptr + i));
			else if(j == 7)
				fprintf(lhd->fp, "%02x -", *(ucptr + i));
			else
				fprintf(lhd->fp, " %02x", *(ucptr + i));
		}
		for(k = j; k < 16; k++)
			fprintf(lhd->fp, "   ");
		if(j < 8)
			fprintf(lhd->fp, " ");
		fprintf(lhd->fp, " ");

		for(k = 0; k < j; k++) {
			if(isprint(*(ucptr+i-j+k)))
				fprintf(lhd->fp, "%c", *(ucptr+i-j+k));
			else
				fprintf(lhd->fp, ".");
		}
		fprintf(lhd->fp, "\n");

		offset += 16;
	}
	fprintf(lhd->fp, "\n");

	LGWR_UNLOCK(lhd->mutex);
}

void lgwr_set_handle(void *hd)
{
	__lgwr__handle = hd;
}
