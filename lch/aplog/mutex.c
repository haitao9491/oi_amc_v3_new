/*
 *
 * mutex.cpp - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#if !defined(OS_WINDOWS)
#include <pthread.h>
#endif

#include "aplog.h"
#include "mutex.h"

struct mutex_hd {
#if !defined(WIN32)
	pthread_mutex_t mutex;
#else
	HANDLE  hMutex;
#endif
};

void *mutex_open(void *t)
{
	struct mutex_hd *mhd;

	if ((mhd = (struct mutex_hd *)malloc(sizeof(*mhd))) == NULL)
		return NULL;

	{
#if !defined(WIN32)
		pthread_mutex_init(&(mhd->mutex), NULL);
#else
		SECURITY_ATTRIBUTES  SecurityAttr;
		char *name = (char *)t;

		SecurityAttr.nLength              = sizeof(SecurityAttr);
		SecurityAttr.lpSecurityDescriptor = NULL;
		SecurityAttr.bInheritHandle       = TRUE;

		mhd->hMutex = CreateMutex(&SecurityAttr, FALSE, (LPCWSTR)name);
		if(mhd->hMutex == NULL) {
			LGWRERROR1(NULL, 0, "CreateMutex(%s) failed.", name);
			free(mhd);
			return NULL;
		}
#endif /* #if !defined(WIN32) */
	}

	return mhd;
}

void mutex_close(void *hd)
{
	struct mutex_hd *mhd = (struct mutex_hd *)hd;

	if (!mhd)
		return;

#if !defined(WIN32)
	pthread_mutex_destroy(&(mhd->mutex));
#else
	CloseHandle(mhd->hMutex);
#endif /* #if !defined(WIN32) */

	free(mhd);
}

/* Return Value:
 * If the specified action, lock or unlock, succeeded, the return
 * value is zero. If an error occurred, the return value is non-zero.
 */
int mutex_lock(void *hd)
{
	struct mutex_hd *mhd = (struct mutex_hd *)hd;

	if (!mhd)
		return -1;

#if !defined(WIN32)
	return pthread_mutex_lock(&(mhd->mutex));
#else
	if(WAIT_OBJECT_0 == WaitForSingleObject(mhd->hMutex, INFINITE))
		return(0);
	LGWRERROR(NULL, 0, "FATAL: WaitForSingleObject(INFINITE) failed.");
	return(-1);
#endif /* #if !defined(WIN32) */
}

int mutex_unlock(void *hd)
{
	struct mutex_hd *mhd = (struct mutex_hd *)hd;

	if (!mhd)
		return -1;

#if !defined(WIN32)
	return pthread_mutex_unlock(&(mhd->mutex));
#else
	if(ReleaseMutex(mhd->hMutex))
		return(0);
	else {
		LGWRERROR(NULL, 0, "ReleaseMutex failed.");
		return(-1);
	}
#endif /* #if !defined(WIN32) */
}
