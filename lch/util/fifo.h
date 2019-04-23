/*
 * (C) Copyright 2010
 * Beijing HLYT Technology Co., Ltd.
 *
 * fifo.h - A brief description to describe this file.
 *
 */

#ifndef _HEAD_FIFO_6709DC94_0643CAFB_4628FB16_H
#define _HEAD_FIFO_6709DC94_0643CAFB_4628FB16_H

#include "os.h"
#include "aplog.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern DLL_APP void *fifo_open(int size, const char *name);
extern DLL_APP int   fifo_push(void *fifo, void *data);
extern DLL_APP void *fifo_pop(void *fifo);
extern DLL_APP int   fifo_get_cnt(void *fifo);
extern DLL_APP void  fifo_clear(void *fifo);
extern DLL_APP void  fifo_close(void *fifo);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_FIFO_6709DC94_0643CAFB_4628FB16_H */
