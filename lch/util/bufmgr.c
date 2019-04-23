/*
 * (C) Copyright 2012
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * bufmgr.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os.h>
#include <aplog.h>
#include "bufmgr.h"


typedef struct {
	char   *next;
} bufmgr_buffer;

typedef struct {
	char   *base;
	int	size;
	int	bufsize;

	int	rbufsize;
	int	rnum;
	int	used;
	char   *lastbuf;		/* last buffer */

	bufmgr_buffer *free;
} bufmgr_handle;


void *bufmgr_open(char *bufmgr, int size, int bufsize)
{
	bufmgr_handle *ctl = NULL;
	int    i, rbufsize, ptrsz = sizeof(void *);
	bufmgr_buffer *buf = NULL;

	if ((bufmgr == NULL) || (bufsize <= 0)) {
		LOGERROR("bufmgr(open): invalid parameters: base, bufsize");
		return NULL;
	}

	rbufsize = sizeof(bufmgr_buffer) + ((bufsize + (ptrsz - 1)) & ~(ptrsz -1));
	if (size < rbufsize) {
		LOGERROR("bufmgr(open): invalid parameters: size");
		return NULL;
	}

	ctl = (bufmgr_handle *)malloc(sizeof(*ctl));
	if (ctl == NULL) {
		LOGERROR("bufmgr(open): failed to allocate handle.");
		return NULL;
	}

	memset(ctl, 0, sizeof(*ctl));
	ctl->base = bufmgr;
	ctl->size = size;
	ctl->bufsize = bufsize;

	ctl->rbufsize = rbufsize;
	ctl->rnum = size / rbufsize;
	ctl->lastbuf = ctl->base + (ctl->rnum - 1) * ctl->rbufsize;
	ctl->free = (bufmgr_buffer *)ctl->base;

	for (i = 0; i < ctl->rnum; i++) {
		buf = (bufmgr_buffer *)(ctl->base + i * ctl->rbufsize);
		
		if (i != ctl->rnum - 1)
			buf->next = ctl->base + (i + 1) * ctl->rbufsize;
		else
			buf->next = NULL;
	}

	return ctl;
}

char *bufmgr_alloc(void *h)
{
	bufmgr_handle *ctl = (bufmgr_handle *)h;
	bufmgr_buffer *buff;
	char  *p;

	if (ctl == NULL)
		return NULL;

	if (ctl->free == NULL)
		return NULL;

	buff = ctl->free;
	ctl->free = (bufmgr_buffer *)(buff->next);
	p = (char *)buff;

	ctl->used++;

	return (p + sizeof(bufmgr_buffer));
}

void bufmgr_free(void *h, char *buf)
{
	bufmgr_handle *ctl = (bufmgr_handle *)h;
	bufmgr_buffer *buff;
	char  *p;

	if ((ctl == NULL) || (buf == NULL))
		return;

	if ((buf < (ctl->base + sizeof(bufmgr_buffer))) || 
	    (buf > (ctl->lastbuf + sizeof(bufmgr_buffer))))
		return;

	p = buf - sizeof(bufmgr_buffer);
	buff = (bufmgr_buffer *)p;

	buff->next = (char *)ctl->free;
	ctl->free = buff;

	ctl->used--;
}

void bufmgr_stat(void *h)
{
	bufmgr_handle *ctl = (bufmgr_handle *)h;

	if (ctl) {
		LOG("bufmgr(stat): base %p, size %d, bufsize %d (real %d): total %d, used %d, free %p",
				ctl->base, ctl->size, ctl->bufsize, ctl->rbufsize,
				ctl->rnum, ctl->used, ctl->free);
	}
}

void bufmgr_close(void *h)
{
	bufmgr_handle *ctl = (bufmgr_handle *)h;

	if (ctl)
		free(ctl);
}

