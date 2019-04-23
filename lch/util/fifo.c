/*
 * (C) Copyright 2010
 * Beijing HLYT Technology Co., Ltd.
 *
 * fifo.c - A brief description to describe this file.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "list.h"
#include "fifo.h"

struct fifo_ctrl {
	int         size;
	const char *name;

	void **elements;
	int    r, w;
};

void *fifo_open(int size, const char *name)
{
	struct fifo_ctrl *ctrl = NULL;

	if (size > 1) {
		ctrl = (struct fifo_ctrl *)malloc(sizeof(struct fifo_ctrl));
		if (ctrl != NULL) {
			ctrl->elements = (void **)malloc(sizeof(void *) * size);
			if (ctrl->elements) {
				ctrl->r = ctrl->w = 0;

				ctrl->size = size;
				ctrl->name = name;
#if 0
				LOG("FIFO[%s]: size %d, created.", ctrl->name, ctrl->size);
#endif
				return ctrl;
			}

			free(ctrl);
			ctrl = NULL;
		}
	}

	LOGERROR("FIFO[%s]: size %d, failed to open.", name, size);
	return ctrl;
}

int fifo_push(void *fifo, void *data)
{
	struct fifo_ctrl *ctrl = (struct fifo_ctrl *)fifo;

	if (ctrl && data) {
		int nw;

		nw = ctrl->w + 1;
		if (nw == ctrl->size)
			nw = 0;

		if (nw != ctrl->r) {
#if 0
			LOGDEBUG("fifo_push: r %d, w %d -> %d, value %p",
					ctrl->r, ctrl->w, nw, data);
#endif
			ctrl->elements[ctrl->w] = data;
			smp_wmb();
			ctrl->w = nw;
			return 0;
		}
	}

	return -1;
}

void *fifo_pop(void *fifo)
{
	struct fifo_ctrl *ctrl = (struct fifo_ctrl *)fifo;

	if (ctrl) {
		int   r = ctrl->r;
		void *p;

		if (r != ctrl->w) {
			p = ctrl->elements[r];
			++r;
			if (r == ctrl->size)
				r = 0;
#if 0
			LOGDEBUG("fifo_pop: r %d -> %d, w %d, value %p",
					ctrl->r, r, ctrl->w, p);
#endif
			ctrl->r = r;

			return p;
		}
	}

	return NULL;
}

void fifo_close(void *fifo)
{
	struct fifo_ctrl *ctrl = (struct fifo_ctrl *)fifo;

	if (ctrl) {
#if 0
		LOG("FIFO[%s]: size %d, released.", ctrl->name, ctrl->size);
#endif
		if (ctrl->elements)
			free(ctrl->elements);
		free(ctrl);
	}
}

void fifo_clear(void *fifo)
{
	struct fifo_ctrl *ctrl = (struct fifo_ctrl *)fifo;

	if (ctrl) {
		ctrl->r = ctrl->w;
	}
}

int fifo_get_cnt(void *fifo)
{
	struct fifo_ctrl *ctrl = (struct fifo_ctrl *)fifo;
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

