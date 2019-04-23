/*
 *
 * cthread.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#if !defined(OS_WINDOWS)
#include <pthread.h>
#endif

#include "aplog.h"
#include "cthread.h"

struct thread_hd {
#if !defined(OS_WINDOWS)
	pthread_t  handle;
	int        id;
#else
	HANDLE     handle;
	DWORD      id;
#endif

	struct thread_arg arg;
};

void *thread_open(LPTHREAD_START_ROUTINE thrfunc, LPVOID arg)
{
	struct thread_hd *thd;

	if ((thd = (struct thread_hd *)malloc(sizeof(*thd))) == NULL)
		return NULL;

	thd->arg.arg = arg;
	thd->arg.flag = 1;

#if !defined(OS_WINDOWS)
	if ((thd->id = pthread_create(&(thd->handle),
					NULL, thrfunc, (LPVOID)&(thd->arg))) != 0)
#else
	if ((thd->handle = CreateThread(NULL,
					0, thrfunc, (LPVOID)&(thd->arg), 0, &(thd->id))) == NULL)
#endif /* #if !defined(OS_WINDOWS) */
	{
		LOGERROR("Failed creating thread %p(%p).", thrfunc, arg);
		free(thd);
		return NULL;
	}

	return thd;
}

void thread_close(void *hd)
{
	struct thread_hd *thd = (struct thread_hd *)hd;

	if (thd) {
		thd->arg.flag = 0;

#if !defined(OS_WINDOWS)
		if (thd->id == 0)
			pthread_join(thd->handle, NULL);
#else
		WaitForSingleObject(thd->handle, INFINITE);
#endif /* #if !defined(OS_WINDOWS) */

		free(thd);
	}
}

int thread_set_concurrency(int newlevel)
{
	int rc = 0;

#if defined(OS_SUN)
	rc = pthread_setconcurrency(newlevel);
	if (rc != 0) {
		LOGERROR("pthread_setconcurrency(%d) failed.", newlevel);
		return -1;
	}
#endif

	return rc;
}
