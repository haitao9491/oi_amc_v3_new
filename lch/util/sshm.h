/*
 * (C) Copyright 2011
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * sshm.h - A description goes here.
 *
 */

#ifndef _HEAD_SSHM_67CA0D2F_254C73D6_27A508BD_H
#define _HEAD_SSHM_67CA0D2F_254C73D6_27A508BD_H

#define SSHM_NORMAL	0x00000000
#define SSHM_CREAT	0x00000001

#if defined(__cplusplus)
extern "C" {
#endif

void *sshm_open(char *name, int size, unsigned int flag);
void *sshm_init(void *hd);
int   sshm_lock(void *hd);
int   sshm_unlock(void *hd);
void  sshm_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_SSHM_67CA0D2F_254C73D6_27A508BD_H */
