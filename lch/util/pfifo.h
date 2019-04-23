/*
 * (C) Copyright 2013
 * Beijing HLYT Technology Co., Ltd.
 *
 * pfifo.h - A brief description to describe this file.
 *
 */

#ifndef _HEAD_PFIFO_6809DC94_0643CAFB_4628FB16_H
#define _HEAD_PFIFO_6809DC94_0643CAFB_4628FB16_H

#include "os.h"
#include "aplog.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern DLL_APP void *pfifo_open(int size, const char *name, int avpsize,
		void *(*mallocp)(int psize), void (*freep)(void *p));

extern DLL_APP void *pfifo_write(void *pfifo, int psize);
extern DLL_APP void  pfifo_write_commit(void *pfifo);
extern DLL_APP void  pfifo_write_rollback(void *pfifo);

extern DLL_APP void *pfifo_read(void *pfifo);
extern DLL_APP void  pfifo_read_commit(void *pfifo);

extern DLL_APP int   pfifo_get_cnt(void *pfifo);
extern DLL_APP void  pfifo_clear(void *pfifo, void (*clearp)(void *p));
extern DLL_APP void  pfifo_close(void *pfifo);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PFIFO_6809DC94_0643CAFB_4628FB16_H */
