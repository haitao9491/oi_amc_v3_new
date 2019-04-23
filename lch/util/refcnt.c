/*
 *
 * refcnt.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#include "aplog.h"
#include "mutex.h"
#include "refcnt.h"

struct refcnt_hd {
	void *lock;
	int   counter;
};

void *refcnt_open(int refcnt)
{
	struct refcnt_hd *hd;

	hd = (struct refcnt_hd *)malloc(sizeof(*hd));
	if (hd == NULL)
		return NULL;

	hd->lock = mutex_open(NULL);
	if (hd->lock == NULL) {
		free(hd);
		return NULL;
	}
	hd->counter = refcnt;

	return hd;
}

int refcnt_add(void *rhd, int refcnt)
{
	int rc;
	struct refcnt_hd *hd = (struct refcnt_hd *)rhd;

	if (hd == NULL)
		return -1;

	mutex_lock(hd->lock);
	hd->counter += refcnt;
	rc = hd->counter;
	mutex_unlock(hd->lock);

	return rc;
}

int refcnt_sub(void *rhd, int refcnt)
{
	return refcnt_add(rhd, -refcnt);
}

int refcnt_inc(void *rhd)
{
	return refcnt_add(rhd, 1);
}

int refcnt_dec(void *rhd)
{
	return refcnt_add(rhd, -1);
}

void refcnt_close(void *rhd)
{
	struct refcnt_hd *hd = (struct refcnt_hd *)rhd;

	if (hd) {
		if (hd->counter) {
			LOGWARN("refcnt: releasing when counter is not zero: %d",
					hd->counter);
		}
		if (hd->lock) {
			mutex_close(hd->lock);
		}
		free(hd);
	}
}
