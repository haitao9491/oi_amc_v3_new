/*
 * (C) Copyright 2010
 * Beijing HLYT Technology Co., Ltd.
 *
 * pfifo.c - A brief description to describe this file.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "list.h"
#include "pfifo.h"

struct pfifo_ctrl {
	int         size;
	int         avpsize;
	const char *name;

	void *(*mallocp)(int psize);
	void (*freep)(void *p);

	void **elements;
	int    r, w;
};

static void *default_mallocp(int psize)
{
	void *p = malloc(psize);

	if (p) {
		memset(p, 0, psize);
	}
	return p;
}

static void default_freep(void *p)
{
	free(p);
}

void *pfifo_open(int size, const char *name, int avpsize,
		void *(*mallocp)(int psize), void (*freep)(void *p))
{
	struct pfifo_ctrl *ctrl = NULL;
	int i;
	void *p = NULL;

	if (size > 1) {
		ctrl = (struct pfifo_ctrl *)malloc(sizeof(struct pfifo_ctrl));
		if (ctrl != NULL) {
			ctrl->elements = (void **)malloc(sizeof(void *) * size);
			if (ctrl->elements) {
				ctrl->r = ctrl->w = 0;

				ctrl->size = size;
				ctrl->avpsize = avpsize;
				ctrl->name = name;

				if (mallocp)
					ctrl->mallocp = mallocp;
				else
					ctrl->mallocp = default_mallocp;

				if (freep)
					ctrl->freep = freep;
				else
					ctrl->freep = default_freep;

				for (i = 0; i < size; ++i) {
					p = ctrl->mallocp(avpsize);
					if (p == NULL) {
						free(ctrl);
						ctrl = NULL;
						LOGERROR("PFIFO[%s]: size %d, avpsize %d,"
								"failed to mallo element [%d].",
								name, size, avpsize, i);
						return ctrl;
					}
					ctrl->elements[i] = p;
#if 0
					LOG("PFIFO[%s]: size %d, created elements[%i] %p.",
							ctrl->name, ctrl->size, i, ctrl->elements[i]);
#endif
				}
				return ctrl;
			}

			free(ctrl);
			ctrl = NULL;
		}
	}

	LOGERROR("PFIFO[%s]: size %d, failed to open.", name, size);
	return ctrl;
}

void *pfifo_write(void *pfifo, int psize)
{
	struct pfifo_ctrl *ctrl = (struct pfifo_ctrl *)pfifo;
	int nw;
	void *p = NULL;

	if (!ctrl)
		return NULL;

	nw = ctrl->w + 1;
	if (nw == ctrl->size)
		nw = 0;

	if (nw == ctrl->r)
		return NULL;

	if (psize <= ctrl->avpsize)
		return ctrl->elements[ctrl->w];

	p = ctrl->mallocp(psize);
	if (p == NULL)
		return NULL;

	ctrl->freep(ctrl->elements[ctrl->w]);
	ctrl->elements[ctrl->w] = p;
	return p;
}

void  pfifo_write_commit(void *pfifo)
{
	struct pfifo_ctrl *ctrl = (struct pfifo_ctrl *)pfifo;
	int nw = 0;

	if (ctrl) {
		smp_wmb();
		nw = ctrl->w + 1;
		if (nw == ctrl->size)
			nw = 0;
		ctrl->w = nw;
	}
}

void  pfifo_write_rollback(void *pfifo)
{
	struct pfifo_ctrl *ctrl = (struct pfifo_ctrl *)pfifo;
	int ow = 0;

	if (ctrl) {
		if (ctrl->w == 0)
			ow = ctrl->size - 1;
		else
			ow = ctrl->w - 1;

		ctrl->w = ow;
	}
}

void *pfifo_read(void *pfifo)
{
	struct pfifo_ctrl *ctrl = (struct pfifo_ctrl *)pfifo;

	if ((ctrl) && (ctrl->r != ctrl->w)) {
		return ctrl->elements[ctrl->r];
	}

	return NULL;
}

void  pfifo_read_commit(void *pfifo)
{
	struct pfifo_ctrl *ctrl = (struct pfifo_ctrl *)pfifo;
	int r = 0;

	if (ctrl) {
		r = ctrl->r;
		++r;
		if (r == ctrl->size)
			r = 0;
		ctrl->r = r;
	}
}

void pfifo_close(void *pfifo)
{
	struct pfifo_ctrl *ctrl = (struct pfifo_ctrl *)pfifo;
	int i;

	if (ctrl) {
#if 0
		LOG("PFIFO[%s]: size %d, released.", ctrl->name, ctrl->size);
#endif
		for (i = 0; i < ctrl->size; ++i)
			ctrl->freep(ctrl->elements[i]);
		if (ctrl->elements)
			free(ctrl->elements);
		free(ctrl);
	}
}

void pfifo_clear(void *pfifo, void (*clearp)(void *p))
{
	struct pfifo_ctrl *ctrl = (struct pfifo_ctrl *)pfifo;
	int i;

	if (ctrl) {
		if (clearp) {
			for (i = 0; i < ctrl->size; ++i)
				clearp(ctrl->elements[i]);
		}
		ctrl->r = ctrl->w;
	}
}

int pfifo_get_cnt(void *pfifo)
{
	struct pfifo_ctrl *ctrl = (struct pfifo_ctrl *)pfifo;
	int r, w;

	if (ctrl) {
		r = ctrl->r;
		w = ctrl->w;
		if (r <= w)
			return (w - r);
		else
			return (ctrl->size - (r - w));
	}

	return 0;
}

