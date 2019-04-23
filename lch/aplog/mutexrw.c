/*
 * (C) Copyright 2007
 * Jie Wang <jie.wang@hlytec.com>
 *
 * mutexrw.cpp - A brief description goes here.
 *
 */

#include <stdlib.h>
#if !defined(OS_WINDOWS)
#include <pthread.h>
#endif
#include "mutexrw.h"

void *mutexrw_open(void *t)
{
	pthread_rwlock_t *lock;

	lock = (pthread_rwlock_t *)malloc(sizeof(*lock));
	if (lock == NULL)
		return NULL;

	pthread_rwlock_init(lock, NULL);

	return lock;
}

void mutexrw_close(void *hd)
{
	pthread_rwlock_destroy((pthread_rwlock_t *)hd);
	free(hd);
}

int mutexrw_rlock(void *hd)
{
	return pthread_rwlock_rdlock((pthread_rwlock_t *)hd);
}

int mutexrw_wlock(void *hd)
{
	return pthread_rwlock_wrlock((pthread_rwlock_t *)hd);
}

int mutexrw_unlock(void *hd)
{
	return pthread_rwlock_unlock((pthread_rwlock_t *)hd);
}
