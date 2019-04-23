/*
 * (C) Copyright 2012
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * bufmgr.h - A description goes here.
 *
 */

#ifndef _HEAD_BUFMGR_4EE871C1_0A4068CC_120720D2_H
#define _HEAD_BUFMGR_4EE871C1_0A4068CC_120720D2_H

#if defined(__cplusplus)
extern "C" {
#endif

void *bufmgr_open(char *bufmgr, int size, int bufsize);
char *bufmgr_alloc(void *h);
void  bufmgr_free(void *h, char *buf);
void  bufmgr_stat(void *h);
void  bufmgr_close(void *h);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_BUFMGR_4EE871C1_0A4068CC_120720D2_H */
