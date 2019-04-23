/*
 *
 * lchmm.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lchmm.h"

struct lchmm_handle {
	int type;
	int size;
	int nelement;

	int   n;
	void *buf;

	void *(*init)(void *hd, int size, int nelement);
	void *(*malloc)(void *hd, int nelement);
	void  (*free)(void *hd, void *ptr, int nelement);
	void  (*exit)(void *hd);
};

/* LCHMM_MALLOC: The default memory management interface, using malloc/free.
 */
static void *lchmm_malloc_malloc(void *hd, int nelement)
{
	struct lchmm_handle *lhd = (struct lchmm_handle *)hd;

	return malloc(lhd->size * nelement);
}

static void lchmm_malloc_free(void *hd, void *ptr, int nelement)
{
	free(ptr);
}

/* Public interfaces for user's usage.
 */
void *lchmm_init(int type, int size, int nelement)
{
	struct lchmm_handle *lhd;

	if ((type < LCHMM_MALLOC) || (type > LCHMM_LAST))
		return NULL;

	if ((size <= 0) || (nelement <= 0))
		return NULL;

	if ((lhd = (struct lchmm_handle *)malloc(sizeof(*lhd))) == NULL)
		return NULL;
	memset(lhd, 0, sizeof(*lhd));

	lhd->type = type;
	lhd->size = size;
	lhd->nelement = nelement;

	if (lhd->type == LCHMM_MALLOC) {
		lhd->malloc = lchmm_malloc_malloc;
		lhd->free   = lchmm_malloc_free;
	}
	else {
		free(lhd);
		lhd = NULL;
	}

	return lhd;
}

void *lchmm_nmalloc(void *hd, int nelement)
{
	struct lchmm_handle *lhd = (struct lchmm_handle *)hd;

	if (!lhd || !lhd->malloc)
		return NULL;

	return lhd->malloc(hd, nelement);
}

void *lchmm_malloc(void *hd)
{
	return lchmm_nmalloc(hd, 1);
}

void lchmm_nfree(void *hd, void *ptr, int nelement)
{
	struct lchmm_handle *lhd = (struct lchmm_handle *)hd;

	if (!lhd || !lhd->free)
		return;

	lhd->free(hd, ptr, nelement);
}

void lchmm_free(void *hd, void *ptr)
{
	lchmm_nfree(hd, ptr, 1);
}

void lchmm_exit(void *hd)
{
	struct lchmm_handle *lhd = (struct lchmm_handle *)hd;

	if (!lhd || !lhd->exit)
		return;

	lhd->exit(hd);
	free(hd);
}

